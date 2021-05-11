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
  sst.setStatisticOutput('sst.statOutputConsole')

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
