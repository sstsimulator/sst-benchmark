/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * - Neither the name of prim nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific prior
 * written permission.
 *
 * See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "sst/core/sst_config.h"

#include "sst/benchmark/Worker.h"

namespace SST {
namespace Benchmark {

void Worker::Event::serialize_order(
    SST::Core::Serialization::serializer& _serializer) {
  SST::Event::serialize_order(_serializer);
  _serializer & time;
  _serializer & count;
};

Worker::Worker(SST::ComponentId_t _id, SST::Params& _params)
  : SST::Component(_id) {
  // Configures output.
  uint32_t verbosity = _params.find<uint32_t>("verbosity", 0);
  output_.init("[@t] Benchmark." + getName() + ": ", verbosity, 0,
               SST::Output::STDOUT);

  // Retrieves parameter values.
  worker_id_ = _params.find<uint32_t>("worker_id");
  output_.verbose(CALL_INFO, 2, 0, "worker_id=%u\n", worker_id_);
  num_peers_ = _params.find<uint32_t>("num_peers");
  output_.verbose(CALL_INFO, 2, 0, "num_peers=%u\n", num_peers_);
  tx_peers_ = _params.find<uint32_t>("tx_peers");
  output_.verbose(CALL_INFO, 2, 0, "tx_peers=%u\n", tx_peers_);
  sst_assert(num_peers_ >= tx_peers_, CALL_INFO, -1,
             "num_peers_ must be >= tx_peers_\n");
  initial_events_ = _params.find<uint32_t>("initial_events", 1);
  output_.verbose(CALL_INFO, 2, 0, "initial_events=%u\n", initial_events_);
  remote_probability_ = _params.find<double>("remote_probability", 0.5);
  output_.verbose(CALL_INFO, 2, 0, "remote_probability=%f\n",
                  remote_probability_);
  sst_assert(remote_probability_ >= 0, CALL_INFO, -1,
             "remote_probability must be >= 0\n");
  sst_assert(remote_probability_ <= 1, CALL_INFO, -1,
             "remote_probability must be <= 1\n");
  if (tx_peers_ == 0) {
    sst_assert(remote_probability_ == 0, CALL_INFO, -1,
               "remote_probability must be 0 with num_peers = 0\n");
  }
  num_cycles_ = _params.find<SST::Cycle_t>("num_cycles", 10000);
  sst_assert(num_cycles_ >= 1, CALL_INFO, -1,
             "num_cycles must be >= 1\n");

  // Seeds the random number generator.
  random_.seed(12345678 + getId());

  // Configures the links for all data ports.
  {
    std::string port_name;
    for (int port_num = 0;
         isPortConnected(port_name = "port_" + std::to_string(port_num));
         port_num++) {
      SST::Link* link = configureLink(
          port_name, new SST::Event::Handler<Worker, int>(
              this, &Worker::handleEvent, port_num));
      if (!link) {
        output_.fatal(CALL_INFO, -1, "unable to configure link %u\n", port_num);
      }
      links_.push_back(link);
    }
  }
  {
    int port_num = links_.size();
    std::string port_name = "port_" + std::to_string(port_num);
    sst_assert(!isPortConnected(port_name), CALL_INFO, -1,
               "%s should NOT be connected on worker %s\n",
               port_name.c_str(), getName().c_str());
    SST::Link* link = configureSelfLink(
        port_name, "1ns", new SST::Event::Handler<Worker, int>(
            this, &Worker::handleEvent, port_num));
    if (!link) {
      output_.fatal(CALL_INFO, -1, "unable to configure link %u\n", port_num);
    }
    links_.push_back(link);
  }
  sst_assert(links_.size() == num_peers_ + 1, CALL_INFO, -1, "ERROR\n");

  // Configures the completion port
  {
    sst_assert(!isPortConnected("completion_port"), CALL_INFO, -1,
               "completion port is already connected on worker %s\n",
               getName().c_str());
    completion_link_ = configureSelfLink(
        "completion_port", std::to_string(num_cycles_ + 1) + "ns",
        new SST::Event::Handler<Worker>(this, &Worker::handleCompletion));
    if (!completion_link_) {
      output_.fatal(CALL_INFO, -1, "unable to configure completion link\n");
    }
  }

  // Registers the statistics.
  event_count_ = registerStatistic<uint64_t>("event_count");

  // Sets the time base.
  registerTimeBase("1ns");

  // Tells the simulator not to end without us.
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();
}

Worker::~Worker() {}

void Worker::setup() {
  output_.verbose(CALL_INFO, 1, 0, "Setup() ns=%" PRIu64 "\n",
                  getCurrentSimTimeNano());
  for (int ev = 0; ev < initial_events_; ev++) {
    sendNextEvent();
  }
  completion_link_->send(1, nullptr);
}

void Worker::complete(unsigned int phase) {
  // If we can determine that we are either an all-to-all or a ring,
  // we can print out the event rate.  For any other topology, we'll
  // point the user to the stats file to get event counts.

  // First check for all-to-all
  if ( num_peers_ == tx_peers_ ) {
    if ( phase == 0 ) {
      if ( worker_id_ != 0 ) {
        Worker::Event* event = new Worker::Event();
        event->time = getRunPhaseElapsedRealTime();
        event->count = num_events_;
        links_.at(0)->sendUntimedData(event);
      }
    }
    else if ( phase == 1) {
      if ( worker_id_ == 0 ) {
        // Get my event rate and accumulate all the partial event
        // rates from my peers
        event_rate_ = (double)num_events_ / getRunPhaseElapsedRealTime();
        total_events_ = num_events_;
        bool success = true;
        for ( int i = 0; i < num_peers_; ++i ) {
          Worker::Event* event = static_cast<Worker::Event*>(links_.at(i)->recvUntimedData());
          if ( nullptr == event ) {
            // Did not receive all the messages, so don't print message rate
            success = false;
            break;
          }
          total_events_ += event->count;
          event_rate_ += (double)event->count / event->time;
        }
        if ( success ) {
          print_rate_ = true;
        }
      }
    }
    else {
      // This shouldn't happen if this is really an all-to-all.
      print_rate_ = false;
    }
  } // End all-to-all

  // Check for ring topology
  if ( num_peers_ == 2 && tx_peers_ == 1 ) {
    // Likely the ring topology.  If it's not, we'll detect it as part
    // of the aggregation.

    // Each worker passes results to port 0.  Worker 0 sends a packet
    // that marks the end of the line (we don't know how many total
    // workers there are)
    if ( phase == 0 ) {
      // Everyone but Worker 0 sends their info on port 0.  Worker 0
      // sends a tag packet (count == 0)
      Worker::Event* event = new Worker::Event();
      event->time = getRunPhaseElapsedRealTime();
      event->count = worker_id_ == 0 ? 0 : num_events_;
      links_.at(0)->sendUntimedData(event);

      if ( worker_id_ == 0 ) {
        total_events_ = num_events_;
        event_rate_ = (double)num_events_ / getRunPhaseElapsedRealTime();
      }
    }
    // Continue to pass received data to port 0.  Worker 0 will
    // aggregate information until it sees the event it send to
    // indicate end of stream.  (Note: Need to pass the tag event or
    // complete() phase will end before we can detect we are done.
    else {
      if ( worker_id_ == 0 ) {
        Worker::Event* event = static_cast<Worker::Event*>(links_.at(1)->recvUntimedData());
        if ( event->count == 0 ) {
          // Successfully done with aggregation
          print_rate_ = true;
        }
        else {
          total_events_ += event->count;
          event_rate_ += (double)event->count / event->time;
        }
      }
      else {
        // Pass on any received_events
        SST::Event* ev = links_.at(1)->recvUntimedData();
        if ( ev ) links_.at(0)->sendUntimedData(ev);
      }
    }
  }
}

void Worker::finish() {
  event_count_->addData(num_events_);
  output_.verbose(CALL_INFO, 1, 0, "Finish()\n");
  if ( worker_id_ == 0 ) {
    if ( print_rate_ ) {
      output_.output("Total events = %" PRIu32 "\n",total_events_);
      output_.output("Events per second = %lf\n",event_rate_);
    }
    else {
      output_.output("WARNING: Could not aggregate message rate due to not finding a supported Worker topology.  Please use the stats output file to compute message rate.");
    }
  }
}

void Worker::sendNextEvent() {
  double rand = random_.nextUniform();
  bool remote = rand <= remote_probability_;
  uint64_t link_index;
  if (remote) {
    link_index = random_.generateNextUInt64() % tx_peers_;
  } else {
    link_index = num_peers_;
  }
  SST::Link* link = links_.at(link_index);
  Worker::Event* event = new Worker::Event();
  event->count = 100;
  event->time = 0.0;
  output_.verbose(CALL_INFO, 3, 0, "Sending event, rand=%f remote=%u\n", rand,
                  remote);
  if (remote) {
    link->send(event);
  } else {
    link->send(1, event);
  }
  num_events_++;
}

void Worker::handleEvent(SST::Event* _event, int _port_num) {
  Worker::Event* event = dynamic_cast<Worker::Event*>(_event);
  if (event) {
    output_.verbose(CALL_INFO, 3, 0, "Received event on port %u ns=%" PRIu64 ".\n",
                    _port_num, getCurrentSimTimeNano());
    delete _event;
    if (getCurrentSimTime() < num_cycles_) {
      sendNextEvent();
    }
  } else {
    output_.fatal(CALL_INFO, -1, "Received bad event type on port %u.\n",
                  _port_num);
  }
}

void Worker::handleCompletion(SST::Event* _event) {
  if (_event == nullptr) {
    output_.verbose(CALL_INFO, 3, 0, "Completing @ ns=%" PRIu64 ".\n",
                    getCurrentSimTimeNano());
    primaryComponentOKToEndSim();
  } else {
    output_.fatal(CALL_INFO, -1, "Received bad completion event type.\n");
  }
}

}  // namespace Benchmark
}  // namespace SST
