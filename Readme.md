# PVHSS-RH

## Project Overview
This repository contains the implementation for *Publicly Verifiable Homomorphic Secret Sharing with Context Hiding and Result Hiding* as described in our paper. 


### Features
- C++17 implementation of PVHSS-RH
- Benchmarking utilities

> **Note:** This codebase is for academic and research purposes only. It is not intended for production use and has not been extensively tested for security vulnerabilities.

## Prerequisites
The PVHSS-RH implementation requires the following libraries:
- [NTL 11.5.1](https://libntl.org/) - Number Theory Library
- [RELIC 0.7.0](https://github.com/relic-toolkit/relic) - Pairing toolkit
- [GMP 6.3.0](https://gmplib.org/) - GNU Multiple Precision Arithmetic Library

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Benchmarks

The benchmark layer writes two final CSV tables:

- `benchmarks/results/micro/micro_timing.csv`
- `benchmarks/results/schemes/scheme_timing.csv`

Recommended entry points:

```bash
# Primitive-level timings.
cmake --build build --target microbenchmark_results

# Scheme timings for degree 5 only.
cmake --build build --target scheme_benchmarks_p5

# Scheme timings for the default degree set.
cmake --build build --target scheme_benchmarks

# Both final tables.
cmake --build build --target benchmark_tables
```

For custom scheme runs, call the scheme runner directly:

```bash
bash benchmarks/schemes/run_scheme_benchmarks.sh \
  --build-dir build \
  --degrees 5,10,15 \
  --cyctimes 5 \
  --msg-num 5 \
  --msg-bits 32
```

See [benchmarks/README.md](benchmarks/README.md) for details.
