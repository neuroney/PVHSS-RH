# Benchmarks

This directory contains the performance runners and result tables.  Correctness
checks live under `tests/correctness`; benchmark targets focus on timing.

## Outputs

The final benchmark artifacts are:

- `benchmarks/results/micro/micro_timing.csv`
- `benchmarks/results/schemes/scheme_timing.csv`

## Recommended Commands

Build everything:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Run primitive-level microbenchmarks:

```bash
./run_micro.sh
```

Run scheme benchmarks:

```bash
./run_schemes.sh
```

## Direct Runners

Use the runner scripts when you need custom parameters without editing CMake:

```bash
SAMPLES=5 ITERS=1 ./run_micro.sh
```

```bash
DEGREES=5,10,15 CYCTIMES=5 MSG_NUM=5 MSG_BITS=32 ./run_schemes.sh
```

See [micro/README.md](micro/README.md) and
[schemes/README.md](schemes/README.md) for detailed scheme-benchmark options.

## Root Scripts

The root runner scripts are optional local convenience wrappers. They remain
useful for long `nohup` runs or one-command local reproduction, but the
benchmark executables can also be invoked directly from `build/benchmarks/`.
