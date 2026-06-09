# PVHSS-RH

## Project Overview
This repository contains the implementation for *Publicly Verifiable Homomorphic Secret Sharing with Context Hiding and Result Hiding* as described in our paper. 


### Features
- C++11 implementation of PVHSS-RH
- Benchmarking utilities

> **Note:** This codebase is for academic and research purposes only. It is not intended for production use and has not been extensively tested for security vulnerabilities.

## Prerequisites
The PVHSS-RH implementation requires the following libraries:
- [NTL 11.5.1](https://libntl.org/) - Number Theory Library
- [RELIC 0.7.0](https://github.com/relic-toolkit/relic) - Pairing toolkit
- [GMP 6.3.0](https://gmplib.org/) - GNU Multiple Precision Arithmetic Library

## Build

1. Download the repository and navigate to the project directory:
    ```bash
    cd PVHSS-RH
    ```
2. Install the dependencies [GMP](https://gmplib.org/), [NTL](https://libntl.org/doc/tour-unix.html) and [RELIC](https://github.com/relic-toolkit/relic/wiki/Building) required by PVHSS-RH. On several Ubuntu systems this can be done directly through links above.

3. Build library and executables:
    ```bash
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    ```

4. To run an example:
    ```bash
    build/schemes/group/ot/PiOTGroup
    build/schemes/rlwe/ot/PiOTRLWE
    build/schemes/group/dc/PiDCGroup
    build/schemes/rlwe/dc/PiDCRLWE
    build/schemes/group/vhss/PiVHSSGroup
    build/schemes/rlwe/vhss/PiVHSSRLWE
    build/schemes/group/hss/PiHSSGroup
    build/schemes/rlwe/hss/PiHSSRLWE
    build/schemes/cz/PiCZ
    ```

Benchmarks are enabled by default.  Use `-DPVHSS_BUILD_BENCHMARKS=OFF` when
only the protocol example binaries are needed.  The benchmark layer produces
three final CSV tables:

- `benchmarks/results/micro/micro_timing.csv`
- `benchmarks/results/protocols/protocol_timing.csv`
- `benchmarks/results/overhead/incremental_timing.csv`

Run `cmake --build build --target benchmark_tables` to regenerate the three
full benchmark tables.

The convenience script runs the full benchmark flow:

```bash
./runall.sh
```