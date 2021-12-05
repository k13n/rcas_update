# Reproducibility Package

This page details the steps needed to reproduce the findings of the paper:

- **Inserting Keys into the Robust Content-and-Structure (RCAS) Index**
- Kevin Wellenzohn, Luka Popovic, Michael H. Böhlen, Sven Helmer
- ADBIS 2021


## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Dataset](#dataset)
3. [Compilation](#compilation)
4. [Experiments](#experiments)


## Prerequisites

Software:
- Linux (we used Ubuntu 20.04.1 LTS)
- C++ compiler supporting at least C++11 (we used gcc version 10.2.0)
- ruby (we used version 3.0.0)
- cmake (we used version 3.16.3)
- perf (we used version 5.4.65)

We used the following hardware:
- Virtual server in a cloud environment
- AMD EPCY 7702 CPU with 1MB L2 cache according to `lscpu`
- 8GB of main memory

We collect the following PMU events with `perf`. Make sure your system can
record them (check `perf list`):
- task-clock
- instructions
- cycles
- cache-references
- cache-misses
- branch-instructions
- branch-misses


## Dataset

We use the ServerFarm dataset from [Wellenzohn et al., 2020] and eliminate
duplicates. The dataset (without duplicates) can be downloaded from:
[Zenodo](https://zenodo.org/record/4771703).

```bash
wget https://zenodo.org/record/4771703/files/sf_size.csv
```


## Compilation

RCAS is written in C++. After checking out the source code, it can be compiled
as follows:

```bash
mkdir release
cd release
cmake .. -DCMAKE_BUILD_TYPE=Release
make
cd ..
```


## Experiments

The following scripts take two parameters:

- `--input_file`: This is the full path to the input dataset
- `--output_folder`: This is the full path to folder where the output files are
  written to.

Change the value of these parameters as needed.


### Insertion Speed Experiment

Execute the following command. The results are stored in the file:
`experiments/exp_insertion/summary.txt`

```bash
ruby scripts/benchmark_insertion.rb \
  --input_file $INPUT_FILE \
  --output_folder experiments/exp_insertion
```

The results are stored in the folder `experiments/exp_insertion` in the
following files:

- `main_lr.csv`
- `main_sr.csv`
- `mainaux_lr.csv`
- `mainaux_sr.csv`


### Deletion Speed Experiment

Execute the following command. The results are stored in the file:
`experiments/exp_deletions/summary.txt`

```bash
ruby scripts/benchmark_deletion.rb \
  --input_file $INPUT_FILE \
  --output_folder experiments/exp_deletion
```

The results are stored in the folder `experiments/exp_insertion` in the
following files:

- `main_lr.csv`
- `main_sr.csv`
- `mainaux_lr.csv`
- `mainaux_sr.csv`




### Insertion Query Performance Experiment

Execute the following command:

```bash
ruby scripts/benchmark_insertion_querying.rb \
  --input_file $INPUT_FILE \
  --output_folder experiments/exp_insertion_querying \
  > experiments/exp_insertion_querying/summary.txt
```

The results are stored in the file: `experiments/exp_insertion_querying/summary.txt`


### Deletion Query Performance Experiment

Execute the following command:

```bash
ruby scripts/benchmark_deletion_querying.rb \
  --input_file $INPUT_FILE \
  --output_folder experiments/exp_deletion_querying \
  > experiments/exp_deletion_querying/summary.txt
```

The results are stored in the file: `experiments/exp_deletion_querying/summary.txt`


### Merge Experiment

Execute the following command:

```bash
ruby scripts/benchmark_merging.rb \
  --input_file $INPUT_FILE \
  --output_folder experiments/exp_merging \
  > experiments/exp_merging/summary.txt
```

The results are stored in the file: `experiments/exp_merging/summary.txt`
