# Benchmarks

This directory contains evaluation artifacts.  Correctness checks live under
`tests/` so benchmark targets can focus on performance measurement and result
generation.

## Final Tables

The benchmark layer produces two presentation-ready CSV tables:

- `benchmarks/results/micro/micro_timing.csv`
- `benchmarks/results/protocols/protocol_timing.csv`

Protocol logs and component-level parser output are kept under
`benchmarks/results/protocols/logs/`.

## Microbenchmark

Build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target microbenchmark_results
```

Fast smoke test:

```sh
bash benchmarks/micro/run_microbenchmarks.sh \
  --build-dir build \
  --cheap-samples 1 \
  --cheap-iters 1 \
  --expensive-samples 1 \
  --expensive-iters 1
```

See `benchmarks/micro/README.md` for the measured primitives and output schema.

## Protocol benchmarks

Protocol benchmarks measure each implemented scheme at the protocol phase
level and write the compact timing table:

```sh
cmake --build build --target protocol_benchmarks_p5
```

For the full paper configuration:

```sh
cmake --build build --target protocol_benchmarks
```

The normalized result is written to
`benchmarks/results/protocols/protocol_timing.csv`.
See `benchmarks/protocols/README.md` for target selection and runner options.

To regenerate the final tables on the fast P5 protocol subset:

```sh
cmake --build build --target benchmark_tables_p5
```
