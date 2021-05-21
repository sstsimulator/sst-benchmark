#!/usr/bin/env python3

"""
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
"""

import argparse
import csv
import json
import glob
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

  layouts = [(128, 8)]#[(1024, 1)]
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
        log_file = os.path.join(args.odir, name + '.log')
        stats_file = os.path.join(args.odir, name + '.csv')
        cmd = ''
        if args.mode == 'threads':
          cmd += 'sst -v -n {} '.format(cpus)
        elif args.mode == 'processes':
          cmd += 'mpirun -n {} sst -v '.format(cpus)
        else:
          assert False, 'programmer error :('
        cmd += '{} -- {} {} -i {} -r 1.0 -c 200000'.format(
          args.app, components, stats_file, initial_events)
        task = taskrun.ProcessTask(tm, name, cmd)
        task.stdout_file = log_file
        task.add_condition(taskrun.FileModificationCondition(
          [], [log_file]))

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
        log_file = os.path.join(args.odir, name + '.log')
        stats_file = os.path.join(args.odir, name + '.csv')
        rate = extract_rate(log_file, stats_file, components)
        rate_sum += rate
        print('{} -> {}'.format(name, rate))
      data[-1].append(rate_sum / args.runs)

  print(data)
  data_labels = ['{}x{}'.format(*layout) for layout in layouts]

  with open(os.path.join(args.odir, 'performance.csv'), 'w') as fd:
    print('Benchmark,' + ','.join([str(x) for x in cpus_list]), file=fd)
    for idx in range(len(data)):
      print(data_labels[idx] + ',', file=fd, end='')
      print(','.join([str(x) for x in data[idx]]), file=fd)

  mlp = ssplot.MultilinePlot(plt, cpus_list, data)
  mlp.set_title('SST-Benchmark performance')
  if args.mode == 'threads':
    mlp.set_xlabel('Number of threads')
  elif args.mode == 'processes':
    mlp.set_xlabel('Number of processes')
  else:
    assert False, 'programmer error :('
  mlp.set_xmajor_ticks(len(cpus_list))
  mlp.set_ylabel('Events per second')
  mlp.set_ymin(0)
  mlp.set_data_labels(data_labels)
  mlp.plot(os.path.join(args.odir, 'performance.png'))


def extract_rate(log_file, stats_file, expected_components):
  sim_time = None
  with open(log_file) as fd:
    for line in fd:
      if line.find('Simulation time:') == 0:
        sim_time = float(line.split()[2])
  assert sim_time is not None, log_file

  ext_loc = stats_file.rfind('.')
  assert ext_loc > 0, stats_file
  stats_fmt = stats_file[0:ext_loc] + '*' + stats_file[ext_loc:]
  event_count = 0
  components = 0
  for stats_file_2 in glob.glob(stats_fmt):
    with open(stats_file_2) as fd:
      try:
        stats = csv.DictReader(fd)
        for row in stats:
          event_count += int(row[' Count.u64'])
          components += 1
      except Exception as ex:
        print(stats_file_2)
        raise ex
  assert components == expected_components, stats_file
  return event_count / sim_time

if __name__ == '__main__':
  ap = argparse.ArgumentParser()
  ap.add_argument('app', help='App to be run by app')
  ap.add_argument('mode', choices=['threads', 'processes'],
                  help='Mode of operations')
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
