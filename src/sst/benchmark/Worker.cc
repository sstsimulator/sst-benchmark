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
#include "sst/benchmark/Worker.h"

namespace SST {
namespace Benchmark {

void Worker::Event::serialize_order(
    SST::Core::Serialization::serializer& _serializer) {
  printf("serializing\n");
  Event::serialize_order(_serializer);
  _serializer & payload;
};

Worker::Worker(SST::ComponentId_t _id, SST::Params& _params)
    : SST::Component(_id) {
  // Configures output.
  output_.init("[@t] Benchmark." + getName() + ": ", 0, 0,
               SST::Output::STDOUT);

  // Retrieves parameter values.
  num_workers_ = _params.find<uint32_t>("num_workers");
  output_.verbose(CALL_INFO, 2, 0, "num_workers=%u\n", num_workers_);
  sst_assert(num_workers_ > 0, CALL_INFO, -1, "num_workers must be > 0\n");
  initial_events_ = _params.find<uint32_t>("initial_events", 1);
  output_.verbose(CALL_INFO, 2, 0, "initial_events=%u\n", initial_events_);
  stagger_time_ = _params.find<bool>("stagger_time", false);
  output_.verbose(CALL_INFO, 2, 0, "stagger_time=%u\n", stagger_time_);
  look_ahead_ = _params.find<SST::Cycle_t>("look_ahead", 1);
  output_.verbose(CALL_INFO, 2, 0, "look_ahead=%lu\n", look_ahead_);
  sst_assert(look_ahead_ > 0, CALL_INFO, -1, "look_ahead must be > 0\n");
  remote_probability_ = _params.find<double>("remote_probability", 0.5);
  output_.verbose(CALL_INFO, 2, 0, "remote_probability=%f\n",
                  remote_probability_);
  sst_assert(remote_probability_ >= 0, CALL_INFO, -1,
             "remote_probability must be >= 0\n");
  sst_assert(remote_probability_ <= 1, CALL_INFO, -1,
             "remote_probability must be <= 1\n");
  if (num_workers_ == 1) {
    sst_assert(remote_probability_ == 0, CALL_INFO, -1,
               "remote_probability must be 0 with num_workers = 1\n");
  }
  num_cycles_ = _params.find<SST::Cycle_t>("num_cycles", 10000);
  sst_assert(num_cycles_ >= look_ahead_, CALL_INFO, -1,
             "num_cycles must be >= look_ahead\n");

  // Seeds the random number generator.
  random_.seed(12345678 + getId());

  // Configures the links for all ports.
  for (int port_num = 0; port_num < num_workers_; port_num++) {
    std::string port_name = "port_" + std::to_string(port_num);
    SST::Link* link = nullptr;
    if (port_num != _id) {
      sst_assert(isPortConnected(port_name), CALL_INFO, -1,
                 "%s should be connected on worker %s\n",
                 port_name.c_str(), getName().c_str());
      link = configureLink(
          port_name, new SST::Event::Handler<Worker, int>(
              this, &Worker::handleEvent, port_num));
      if (!link) {
        output_.fatal(CALL_INFO, -1, "unable to configure link %u\n", port_num);
      }
      links_.push_back(link);
    }
  }
  {
    int port_num = getId();
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
  sst_assert(links_.size() == num_workers_, CALL_INFO, -1, "ERROR\n");

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
  output_.verbose(CALL_INFO, 1, 0, "Setup() ns=%lu\n",
                  getCurrentSimTimeNano());
  for (int ev = 0; ev < initial_events_; ev++) {
    sendNextEvent();
  }
}

void Worker::finish() {
  output_.verbose(CALL_INFO, 1, 0, "Finish()\n");
}

//Worker::Worker() : SST::Component(-1) {}

void Worker::sendNextEvent() {
  double rand = random_.nextUniform();
  bool remote = rand <= remote_probability_;
  uint64_t link_index;
  if (remote) {
    link_index = random_.generateNextUInt64() % (num_workers_ - 1);
  } else {
    link_index = num_workers_ - 1;
  }
  SST::Link* link = links_.at(link_index);
  Worker::Event* event = new Worker::Event();
  event->payload = 'a';
  output_.verbose(CALL_INFO, 3, 0, "Sending event, rand=%f remote=%u\n", rand,
                  remote);
  if (remote) {
    link->send(event);
  } else {
    link->send(1, event);
  }
  event_count_->addData(1);
}

void Worker::handleEvent(SST::Event* _event, int _port_num) {
  Worker::Event* event = dynamic_cast<Worker::Event*>(_event);
  if (event) {
    output_.verbose(CALL_INFO, 3, 0, "Received event on port %u ns=%lu.\n",
                    _port_num, getCurrentSimTimeNano());
    delete _event;
    if (getCurrentSimTime() < num_cycles_) {
      sendNextEvent();
    } else {
      primaryComponentOKToEndSim();
    }
  } else {
    output_.fatal(CALL_INFO, -1, "Received bad event type on port %u.\n",
                  _port_num);
  }
}

}  // namespace Benchmark
}  // namespace SST
