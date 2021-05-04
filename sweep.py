#!/usr/bin/env python3

import argparse
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy
import os
import ssplot
import taskrun

def main(args):
  assert os.path.isfile(args.app)
  if not os.path.isdir(args.odir):
    os.mkdir(args.odir)

  rm = taskrun.ResourceManager(
    taskrun.CounterResource('cpus', os.cpu_count(), os.cpu_count()))
  cob = taskrun.FileCleanupObserver()
  vob = taskrun.VerboseObserver(show_description=args.verbose)
  tm = taskrun.TaskManager(resource_manager=rm,
                           observers=[cob, vob],
                           failure_mode=taskrun.FailureMode.AGGRESSIVE_FAIL)

  cpus_start = 1
  cpus_stop = os.cpu_count()
  cpus_step = args.step
  cpus_list = [x for x in range(cpus_start, cpus_stop + 1)
               if x % cpus_step == 0]
  for run in range(args.runs):
    for cpus in cpus_list:
      name = '{}_{}'.format(cpus, run)
      ofile = os.path.join(args.odir, name + '.log')
      cmd = 'sst -v -n {} {} -- 1000 -i 1 -c 100000'.format(cpus, args.app)
      task = taskrun.ProcessTask(tm, name, cmd)
      task.stdout_file = ofile
      task.add_condition(taskrun.FileModificationCondition(
        [], [ofile]))

  tm.randomize()
  res = tm.run_tasks()
  if not res:
    return -1

  data = {}
  for run in range(args.runs):
    for cpus in cpus_list:
      name = '{}_{}'.format(cpus, run)
      ofile = os.path.join(args.odir, name + '.log')
      rate = extract_rate(ofile)
      print('{} -> {}'.format(name, rate))
      if cpus not in data:
        data[cpus] = 0
      data[cpus] += rate
  for cpus in cpus_list:
    data[cpus] /= args.runs

  print(data)

  x = sorted(list(data.keys()))
  y = [data[k] for k in x]
  mlp = ssplot.MultilinePlot(plt, x, [y])
  mlp.set_title('SST-Benchmark performance')
  mlp.set_xlabel('Number of threads')
  mlp.set_xmajor_ticks(len(cpus_list))
  mlp.set_ylabel('Events per second')
  mlp.plot(os.path.join(args.odir, 'performance.png'))


def extract_rate(filename):
  sim_time = None
  event_count = 0
  worker_count = 0
  with open(filename) as fd:
    for line in fd:
      if line.find('Simulation time:') == 0:
        sim_time = float(line.split()[2])
      elif line.find('Count.u64') >= 0:
        worker_count += 1
        event_count += int(line.split()[12][0:-1])
  assert sim_time is not None
  assert worker_count == 1000
  return event_count / sim_time

if __name__ == '__main__':
  ap = argparse.ArgumentParser()
  ap.add_argument('app', help='App to be run by app')
  ap.add_argument('odir', help='Output directory')
  ap.add_argument('-r', '--runs', type=int, default=1,
                  help='Number of runs')
  ap.add_argument('-s', '--step', type=int, default=4,
                  help='Step size for number of cpus (threads)')
  ap.add_argument('-v', '--verbose', action='store_true',
                  help='Show task descriptions')
  args = ap.parse_args()
  main(args)
