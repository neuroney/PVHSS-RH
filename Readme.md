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

The benchmark layer writes two CSV tables:

- `benchmarks/results/micro/micro_timing.csv`
- `benchmarks/results/schemes/scheme_timing.csv`

### Running benchmarks

```bash
# 1. Build binaries
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 2. Micro-benchmarks (primitive-level timings)
./run_micro.sh

# 3. Scheme benchmarks — 10 schemes × degrees 5,10,15
./run_schemes.sh
```

### Scheme targets

| Family  | Targets |
|---------|---------|
| Group   | `scheme_group_tcc25` `scheme_group_hss` `scheme_group_vhss` `scheme_group_ot` `scheme_group_dc` |
| RLWE    | `scheme_rlwe_cz` `scheme_rlwe_hss` `scheme_rlwe_vhss` `scheme_rlwe_ot` `scheme_rlwe_dc` |