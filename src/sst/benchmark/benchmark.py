import argparse
import sst

def main(args):
  print('\nStarting SST benchmark')

  # Makes workers and sets parameters.
  workers = []
  for worker_id in range(args.num_workers):
    worker = sst.Component('Worker_{}'.format(worker_id), 'benchmark.Worker')
    worker.addParam('num_workers', args.num_workers)
    if args.initial_events != None:
      worker.addParam('initial_events', args.initial_events)
    if args.stagger_events != None:
      worker.addParam('stagger_events', args.stagger_events)
    if args.look_ahead != None:
      worker.addParam('look_ahead', args.look_ahead)
    if args.remote_probability != None:
      worker.addParam('remote_probability', args.remote_probability)
    if args.num_cycles != None:
      worker.addParam('num_cycles', args.num_cycles)
    workers.append(worker)

  # Connects all workers via links.
  for worker_a in range(args.num_workers):
    for worker_b in range(worker_a, args.num_workers):
      link_name = 'link_{}_{}'.format(worker_a, worker_b)
      if worker_a != worker_b:  # TODO(nicmcd): remove 'if' once self-links work
        link = sst.Link(link_name, '1ns')
        link.connect((workers[worker_a], 'port_{}'.format(worker_b)),
                     (workers[worker_b], 'port_{}'.format(worker_a)))

  # Limits the verbosity of statistics to any with a load level from 0-7.
  sst.setStatisticLoadLevel(7)

  # Determines where statistics should be sent.
  sst.setStatisticOutput('sst.statOutputCSV')

  # Enables statistics on both workers.
  sst.enableAllStatisticsForComponentType('benchmark.Worker')

if __name__ == '__main__':
  ap = argparse.ArgumentParser()
  ap.add_argument('num_workers', type=int, default=10,
                  help='Number of workers.')
  ap.add_argument('-i', '--initial_events', type=int,
                  help='Number of initial events per worker.')
  ap.add_argument('-s', '--stagger_events', type=bool,
                  help='Enable staggered time execution.')
  ap.add_argument('-l', '--look_ahead', type=int,
                  help='Amount of event look ahead in cycles.')
  ap.add_argument('-r', '--remote_probability', type=float,
                  help='Probility of for each event to a remote event [0-1].')
  ap.add_argument('-c', '--num_cycles', type=int,
                  help='Number of cycles to simulate')
  args = ap.parse_args()
  main(args)
