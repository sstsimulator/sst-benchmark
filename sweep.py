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

  cpus_list = [x for x in range(args.start, args.stop+1, args.step)]

  layouts = [(1024, 1)]
  while True:
    if layouts[-1][0] <= args.stop:
      break
    layouts.append((layouts[-1][0] // 2, layouts[-1][1] * 2),)
  print(layouts)

  for layout in layouts:
    components = layout[0]
    initial_events = layout[1]
    for cpus in cpus_list:
      for run in range(args.runs):
        name = '{}_{}_{}_{}'.format(components, initial_events, cpus, run)
        ofile = os.path.join(args.odir, name + '.log')
        cmd = 'sst -v -n {} {} -- {} -i {} -r 1.0 -c 200000'.format(
          cpus, args.app, components, initial_events)
        task = taskrun.ProcessTask(tm, name, cmd)
        task.stdout_file = ofile
        task.add_condition(taskrun.FileModificationCondition(
          [], [ofile]))

  tm.randomize()
  res = tm.run_tasks()
  if not res:
    return -1

  data = []
  for layout in layouts:
    components = layout[0]
    initial_events = layout[1]
    layout_str = '{}x{}'.format(*layout)
    data.append([])
    for cpus in cpus_list:
      rate_sum = 0
      for run in range(args.runs):
        name = '{}_{}_{}_{}'.format(components, initial_events, cpus, run)
        ofile = os.path.join(args.odir, name + '.log')
        rate = extract_rate(ofile, components)
        rate_sum += rate
        print('{} -> {}'.format(name, rate))
      data[-1].append(rate_sum / args.runs)

  print(data)

  mlp = ssplot.MultilinePlot(plt, cpus_list, data)
  mlp.set_title('SST-Benchmark performance')
  mlp.set_xlabel('Number of threads')
  mlp.set_xmajor_ticks(len(cpus_list))
  mlp.set_ylabel('Events per second')
  mlp.set_ymin(0)
  mlp.set_data_labels(['{}x{}'.format(*layout) for layout in layouts])
  mlp.plot(os.path.join(args.odir, 'performance.png'))


def extract_rate(filename, expected_components):
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
  assert worker_count == expected_components
  return event_count / sim_time

if __name__ == '__main__':
  ap = argparse.ArgumentParser()
  ap.add_argument('app', help='App to be run by app')
  ap.add_argument('odir', help='Output directory')
  ap.add_argument('start', type=int, help='starting cpus')
  ap.add_argument('stop', type=int, help='stopping cpus')
  ap.add_argument('step', type=int, help='cpus step')
  ap.add_argument('-r', '--runs', type=int, default=1,
                  help='Number of runs')
  ap.add_argument('-v', '--verbose', action='store_true',
                  help='Show task descriptions')
  args = ap.parse_args()
  main(args)
