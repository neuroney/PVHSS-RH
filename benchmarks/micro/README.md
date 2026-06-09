# Microbenchmark

The microbenchmark executables measure primitive-level costs used in the
performance discussion.  Use the runner for the final table:

```sh
python3 benchmarks/micro/run_microbenchmarks.py --build-dir build
```

It writes `benchmarks/results/micro/micro_timing.csv` with one row per
operation:

```text
category,operation,mean_ms
```

The runner uses `std::chrono::steady_clock`.  Cheap operations are timed with
adaptive batching until a sample reaches `--min-sample-ms`; expensive
operations use fixed iteration counts so long RLWE and HSS paths can run once.
By default, cheap operations use 10 samples and expensive operations use 1
sample.  The reported `mean_ms` is the average per operation after dividing
each sample by the number of iterations used for that sample.
The runner combines `microbench` with the separate `microbench_vhss_rlwe`
binary, which avoids symbol collisions between the RLWE HSS and VHSS source
files.

Measured primitives include:

- HSS group AddMemory and Multiply
- VHSS group AddMemory and Multiply
- HSS RLWE AddMemory, Enc/OKDM, and Multiply/DDec
- VHSS RLWE AddMemory, Enc/OKDM, and Multiply/DDec
- Pairing, G1 exponentiation, G2 exponentiation, GT exponentiation
- Pedersen commitment, DecPed commitment, DecPed decryption
- PRF_ZZ and PRF_bn

Useful knobs include `--cheap-samples`, `--cheap-iters`,
`--min-sample-ms`, `--max-adaptive-iters`, `--expensive-samples`, and
`--expensive-iters`.
