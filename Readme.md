# PVHSS-RH

## Project Overview
This repository contains the implementation for Publicly Verifiable Homomorphic Secret Sharing with Result Hiding (PVHSS-RH) as described in our paper. 


### Features
- C++11 implementation of PVHSS-RH
- Benchmarking utilities

> **Note:** This codebase is for academic and research purposes only. It is not intended for production use and has not been extensively tested for security vulnerabilities.

## Prerequisites
The PVHSS-RH implementation requires the following libraries:
- [NTL 11.5.1](https://libntl.org/) - Number Theory Library
- [RELIC 0.7.0](https://github.com/relic-toolkit/relic) - Pairing toolkit
- [GMP 6.3.0](https://gmplib.org/) - GNU Multiple Precision Arithmetic Library

## Installation

1. Download the repository and navigate to the project directory:
    ```bash
    cd PVHSS-RH
    ```
2. Install the dependencies [GMP](https://gmplib.org/), [NTL](https://libntl.org/doc/tour-unix.html) and [RELIC](https://github.com/relic-toolkit/relic/wiki/Building) required by PVHSS-RH. On several Ubuntu systems this can be done directly through links above.

3. Build library and executables:
    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```
4. To run an example:
    ```bash
    ./GroupPi1/GroupPi1
    ./GroupPi2/GroupPi2
    ```

## Project Structure
- `common/` - Shared utilities and functions
- `GroupPi1/` - Implementation for Group-based $\Pi_1$
- `GroupPi2/` - Implementation for Group-based $\Pi_2$
- `cmake/` - CMake configuration files