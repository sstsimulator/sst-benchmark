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
#ifndef SST_BENCHMARK_WORKER_H_
#define SST_BENCHMARK_WORKER_H_

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/params.h>
#include <sst/core/eli/elementinfo.h>
#include <sst/core/rng/mersenne.h>

#include <cstdint>

namespace SST {
namespace Benchmark {

class Worker : public SST::Component {
 private:
  struct Event : public SST::Event {
    Event() = default;
    ~Event() override = default;
    char payload;
    void serialize_order(SST::Core::Serialization::serializer& _serializer)
        override;
    ImplementSerializable(SST::Benchmark::Worker::Event);
  };

 public:
  Worker(SST::ComponentId_t _id, SST::Params& _params);
  ~Worker() override;

  void setup() override;
  void finish() override;

  SST_ELI_REGISTER_COMPONENT(
      Worker,
      "benchmark",
      "Worker",
      SST_ELI_ELEMENT_VERSION(1,0,0),
      "Benchmark Worker",
      COMPONENT_CATEGORY_UNCATEGORIZED)

  SST_ELI_DOCUMENT_PORTS(
      {"port_%(portnum)d",
       "Links to self or other workers.",
       {"benchmark.Worker"}},
      {"completion_port",
       "Link to self for simulation completion.",
       {"benchmark.Worker"}})

  SST_ELI_DOCUMENT_PARAMS(
      {"verbosity",
       "Level of verbosity.",
       "0"},
      {"num_workers",
       "Number of total workers.",
       NULL},
      {"initial_events",
       "Number of initial events created by this worker.",
       "1"},
      {"stagger_time",
       "Enable staggered time execution.",
       "false"},
      {"look_ahead",
       "Amount of event look ahead measured in cycles.",
       "1"},
      {"remote_probability",
       "Probility of for each event to a remote event [0-1].",
       "0.5"},
      {"num_cycles",
       "Number of cycles to run for.",
       "10000"})

  SST_ELI_DOCUMENT_STATISTICS(
      {"event_count",
       "The count of events generated", "unitless", 1})

 private:
  Worker(const Worker&) = delete;
  void operator=(const Worker&) = delete;

  void sendNextEvent();
  virtual void handleEvent(SST::Event* _event, int _port_num);
  virtual void handleCompletion(SST::Event* _event);

  // Output
  SST::Output output_;

  // Parameters
  uint32_t num_workers_;
  uint32_t initial_events_;
  bool stagger_time_;
  SST::Cycle_t look_ahead_;
  double remote_probability_;
  SST::Cycle_t num_cycles_;

  // Links
  std::vector<SST::Link*> links_;
  SST::Link* completion_link_;

  // Random
  SST::RNG::MersenneRNG random_;

  // Statistics
  SST::Statistics::Statistic<uint64_t>* event_count_;
};

}  // namespace Benchmark
}  // namespace SST

#endif  // SST_BENCHMARK_WORKER_H_
