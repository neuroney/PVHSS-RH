# Microbenchmark

The microbenchmark executables measure primitive-level costs used in the
performance discussion.  Use the runner for the final table:

```sh
bash benchmarks/micro/run_microbenchmarks.sh --build-dir build
```

It writes `benchmarks/results/micro/micro_timing.csv` with one row per
operation:

```text
category,operation,mean_ms
```

The C++ benchmark executables use `std::chrono::steady_clock`.  Cheap operations are timed with
adaptive batching until a sample reaches `--min-sample-ms`; expensive
operations use fixed iteration counts so long RLWE and HSS paths can run once.
By default, cheap operations use 10 samples and expensive operations use 1
sample.  The reported `mean_ms` is the average per operation after dividing
each sample by the number of iterations used for that sample.
The runner executes the single `microbench` binary.  Protocol code is namespaced
so RLWE HSS and VHSS can be linked into the same executable without symbol
collisions.

Measured primitives include:

- HSS group AddMemory and Multiply
- VHSS group AddMemory and Multiply
- HSS RLWE AddMemory, Enc/OKDM, and Multiply/DDec
- VHSS RLWE AddMemory, Enc/OKDM, and Multiply/DDec
- Pairing, G1 exponentiation, G2 exponentiation, GT exponentiation
- Pedersen commitment, DecPed commitment, DecPed decryption at 8/16/32 bits
- PrfZZ and PrfBn

Useful knobs include `--cheap-samples`, `--cheap-iters`,
`--min-sample-ms`, `--max-adaptive-iters`, `--expensive-samples`, and
`--expensive-iters`.
