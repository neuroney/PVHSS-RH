# PVHSS-RH

## Project Overview
This codebase is part of the PVHSS-RH paper.

**What this codebase includes**: example and benchmark implementations in C++11 for the publicly verifiable homomorphic secret sharing scheme 
with result hiding property in the PVHSS paper.

**What this codebase is not**: it is not for production use; it is not extensively tested.

## Table of Contents
- [Installation](#installation)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)

## Installation
To install the project, follow these steps:

1. Download the repository and navigate to the project directory:
    ```bash
    cd PVHSS-RH
    ```
2. Install the dependencies [NTL](https://libntl.org/doc/tour-unix.html) and [relic](https://github.com/relic-toolkit/relic/wiki/Building) required by PVHSS-RT. On several Ubuntu systems this can be done directly through links above.

3. Build library and executables:
    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```
4. To run an example:
    ```bash
    cd build/PVHSS
    ```