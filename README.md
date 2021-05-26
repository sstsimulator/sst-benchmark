# sst-benchmark

SST-Benchmark is a simple benchmark suite for [sst-core](https://github.com/sstsimulator/sst-core). SST-Benchmark also is a template repository for creating a project that depends on SST-Core.

## Instructions
Follow the instructions [here](http://sst-simulator.org/SSTPages/SSTBuildAndInstall10dot1dot0SeriesDetailedBuildInstructions) to install SST-core.

Installing SST-Benchmark is done exactly like SST-Elements:
``` bash
$ cd $HOME/scratch/src
$ git clone https://github.com/nicmcd/sst-benchmark
$ export SST_BENCHMARK_HOME=$HOME/local/sst-benchmark
$ export SST_BENCHMARK_ROOT=$HOME/scratch/src/sst-benchmark
$ cd $HOME/scratch/src/sst-benchmark
$ ./autogen.sh
$ ./configure --with-sst-core=$SST_CORE_HOME --prefix=$SST_BENCHMARK_HOME
$ make all
$ make install
```

Running the SST-Benchmark is as follows:

``` bash
$ cd $HOME/scratch/src/sst-benchmark
$ sst src/sst/benchmark/benchmark.py 10
```

This runs the benchmark using 10 worker components in the "all-to-all" configuration. To get any useful information as a benchmark, you'll need to get SST's execution time and the total number of events. SST will show the execution time when run with `-v`. Each worker logs the number of events it generated during the execution. These are written to "StatisticOutput.csv".

``` bash
$ cd $HOME/scratch/src/sst-benchmark
$ sst -v src/sst/benchmark/benchmark.py all-to-all 10 | grep 'Simulation time:'
0.022400 seconds
$ awk '{print $9}' StatisticOutput.csv | tr ',' ' ' | awk '{s+=$1} END {print s}'
100000
$ bc -l <<< "100000 / 0.022400"
4464285.71428571428571428571
```

This example shows a total execution speed of ~4.5M events per second.

To run a full sweep of multithreaded simulations, run the sweep script:
``` bash
$ cd $HOME/scratch/src/sst-benchmark
$ ./sweep.py src/sst/benchmark/benchmark.py all-to-all threads output1 1 `nproc` 1 -r 3 -v
$ eog output1/performance.png
```
**Note:** ``1 `nproc` 1`` tests every hardware thread on the machine. For example if nproc is 4, then the script will test 1, 2, 3, and 4 threads.

**Note:** `-r 3` runs each sample 3 times and averages the results.

To run a full sweep of multiprocess simulations, change `threads` to `processes`:
``` bash
$ cd $HOME/scratch/src/sst-benchmark
$ ./sweep.py src/sst/benchmark/benchmark.py all-to-all processes output2 1 `nproc` 1 -r 3 -v
$ eog output2/performance.png
```

**TODO (nicmcd):** combine threads and processes into a single run so that they can be plotted against each other and that hybrid process+thread runs can be tested.

To run a full sweep of multiprocess simulations using the "ring" configuration, change `all-to-all` to `ring`:
``` bash
$ cd $HOME/scratch/src/sst-benchmark
$ ./sweep.py src/sst/benchmark/benchmark.py ring processes output3 1 `nproc` 1 -r 3 -v
$ eog output3/performance.png
```
