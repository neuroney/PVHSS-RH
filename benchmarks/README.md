# Benchmarks

This directory contains the performance runners and result tables.  Correctness
checks live under `tests/correctness`; benchmark targets focus on timing.

## Outputs

The final benchmark artifacts are:

- `benchmarks/results/micro/micro_timing.csv`
- `benchmarks/results/schemes/scheme_timing.csv`

Per-run scheme logs are written under
`benchmarks/results/schemes/logs/`.

## Recommended Commands

Build everything:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Run primitive-level microbenchmarks:

```bash
cmake --build build --target microbenchmark_results
```

Run scheme benchmarks:

```bash
# Degree 5 smoke/presentation subset.
cmake --build build --target scheme_benchmarks_p5

# Default scheme degree set from the runner.
cmake --build build --target scheme_benchmarks
```

Run both final benchmark tables:

```bash
cmake --build build --target benchmark_tables
```

## Direct Runners

Use the runner scripts when you need custom parameters without editing CMake:

```bash
bash benchmarks/micro/run_microbenchmarks.sh \
  --build-dir build \
  --samples 5 \
  --iters 1
```

```bash
bash benchmarks/schemes/run_scheme_benchmarks.sh \
  --build-dir build \
  --degrees 5,10,15 \
  --cyctimes 5 \
  --msg-num 5 \
  --msg-bits 32
```

See [micro/README.md](micro/README.md) and
[schemes/README.md](schemes/README.md) for detailed scheme-benchmark options.

## `runall.sh`

`runall.sh` is an optional local convenience wrapper.  It remains useful for
long `nohup` runs or one-command local reproduction, but it is not required:

- CMake targets can run the benchmark tables directly.
- `benchmarks/micro/run_microbenchmarks.sh` accepts CLI flags and environment
  variables.
- `benchmarks/schemes/run_scheme_benchmarks.sh` accepts CLI flags and
  environment variables.
