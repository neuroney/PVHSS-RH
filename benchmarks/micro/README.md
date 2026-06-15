# Microbenchmarks

The microbenchmark executable measures primitive-level costs used in the
performance discussion.

## Run

Build and run through CMake:

```bash
cmake --build build --target microbench
./run_micro.sh
```

Or call the runner directly:

```bash
SAMPLES=5 ITERS=1 ./run_micro.sh
```

For lower-level tuning, call the executable directly:

```bash
build/benchmarks/micro/microbench \
  --samples 3 \
  --iters 1 \
  --min-sample-ms 25
```

Defaults can also be set with environment variables:

- `SAMPLES`
- `ITERS`

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
