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

This runs the benchmark using 10 worker components. To get any useful information as a benchmark, you'll need to get SST's execution time and the total number of events. SST will show the execution time when run with `-v`. Each worker logs the number of events it generated during the execution. These are written to "StatisticOutput.csv".

``` bash
$ cd $HOME/scratch/src/sst-benchmark
$ sst -v src/sst/benchmark/benchmark.py 10 | grep 'Simulation time:'
0.022400 seconds
$ awk '{print $9}' StatisticOutput.csv | tr ',' ' ' | awk '{s+=$1} END {print s}'
100000
$ bc -l <<< "100000 / 0.022400"
4464285.71428571428571428571
```

This example shows a total execution speed of ~4.5M events per second.
