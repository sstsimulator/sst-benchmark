// Copyright (c) 2021, Nic McDonald
// TBD
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
         {"benchmark.Worker"}})

  SST_ELI_DOCUMENT_PARAMS(
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
  //Worker();  // For serialization only
  //Worker(const Worker&) = delete;
  //void operator=(const Worker&) = delete;

  void sendNextEvent();
  virtual void handleEvent(SST::Event* _event, int _port_num);

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

  // Random
  SST::RNG::MersenneRNG random_;

  // Statistics
  SST::Statistics::Statistic<uint64_t>* event_count_;
};

}  // namespace Benchmark
}  // namespace SST

#endif  // SST_BENCHMARK_WORKER_H_
