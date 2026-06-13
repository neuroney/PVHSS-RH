# Microbenchmarks

The microbenchmark executable measures primitive-level costs used in the
performance discussion.

## Run

Build and run through CMake:

```bash
cmake --build build --target microbenchmark_results
```

Or call the runner directly:

```bash
bash benchmarks/micro/run_microbenchmarks.sh \
  --build-dir build \
  --samples 5 \
  --iters 1
```

The runner forwards unknown options to `microbench`, so knobs such as
`--min-sample-ms` and `--max-adaptive-iters` can be passed through:

```bash
bash benchmarks/micro/run_microbenchmarks.sh \
  --build-dir build \
  --samples 3 \
  --iters 1 \
  --min-sample-ms 25
```

Defaults can also be set with environment variables:

- `MICRO_SAMPLES`
- `MICRO_ITERS`

Use CLI flags or environment variables for direct microbenchmark runs.

## Output

The output table is:

```text
benchmarks/results/micro/micro_timing.csv
```

Schema:

```text
category,operation,mean_ms
```

## Measured Primitives

- HSS group AddMemory and Multiply
- VHSS group AddMemory and Multiply
- HSS RLWE AddMemory, Enc/OKDM, and Multiply/DDec
- VHSS RLWE AddMemory, Enc/OKDM, and Multiply/DDec
- Pairing, G1 exponentiation, G2 exponentiation, GT exponentiation
- Pedersen commitment, DecPed commitment, DecPed decryption at 8/16/32 bits
- `PrfZZ` and `PrfBn`

The C++ benchmark executable uses `std::chrono::steady_clock`.  Fast operations
can use adaptive batching until a sample reaches `--min-sample-ms`; long
operations use fixed iteration counts.
