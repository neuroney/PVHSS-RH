# PVHSS-RH

## Project Overview
PVHSS-RH is a CMake-based C++17 research artifact for publicly verifiable homomorphic secret sharing with context and result hiding. This repository accompanies the following paper:

> Jinyuan Li and Liang Feng Zhang. "Publicly Verifiable Homomorphic Secret Sharing with Context and Result Hiding." *IEEE Transactions on Information Forensics and Security*, 2026. [https://doi.org/10.1109/TIFS.2026.3744639](https://doi.org/10.1109/TIFS.2026.3744639).

The repository includes group-based and RLWE-based implementations, comparison schemes, and benchmarks for primitive operations, scheme evaluation, and private information retrieval (PIR).

### Features
- Group-based and RLWE-based implementations of PVHSS-RH
- HSS, VHSS, and comparison scheme implementations
- Benchmark drivers and CSV timing outputs for experimental evaluation and reproducibility

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

The benchmark layer writes three CSV tables:

- `benchmarks/results/micro/micro_timing.csv`
- `benchmarks/results/schemes/scheme_timing.csv`
- `benchmarks/results/pir/pir_group_dc_timing.csv`

### Running benchmarks

```bash
# 1. Build binaries
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 2. Micro-benchmarks (primitive-level timings)
./run_micro.sh

# 3. Scheme benchmarks — 10 schemes × degrees 5,10,15
./run_schemes.sh

# 4. Group-DC PIR benchmarks
./run_pir.sh
```

### Scheme targets

| Family  | Targets |
|---------|---------|
| Group   | `scheme_group_tcc25` `scheme_group_hss` `scheme_group_vhss` `scheme_group_ot` `scheme_group_dc` |
| RLWE    | `scheme_rlwe_cz` `scheme_rlwe_hss` `scheme_rlwe_vhss` `scheme_rlwe_ot` `scheme_rlwe_dc` |

## Citation

If you use this repository in academic work, please cite:

```bibtex
@article{li2026pvhss,
  author = {Jinyuan Li and Liang Feng Zhang},
  title = {Publicly Verifiable Homomorphic Secret Sharing with Context and Result Hiding},
  journal = {IEEE Transactions on Information Forensics and Security},
  year = {2026},
  doi = {10.1109/TIFS.2026.3744639},
  url = {https://doi.org/10.1109/TIFS.2026.3744639}
}
```

## License

Released under the MIT License. See [LICENSE](LICENSE) for details.
