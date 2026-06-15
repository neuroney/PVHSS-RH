# Scheme Benchmarks

Scheme benchmarks measure setup, input generation, server computation,
verification, and decode costs for each implemented scheme/backend pair.

## Targets

The runner covers these benchmark binaries:

- `scheme_group_hss`
- `scheme_group_vhss`
- `scheme_group_ot`
- `scheme_group_dc`
- `scheme_group_tcc25`
- `scheme_rlwe_hss`
- `scheme_rlwe_vhss`
- `scheme_rlwe_ot`
- `scheme_rlwe_dc`
- `scheme_rlwe_cz`

## CMake Entry Points

```bash
cmake --build build
./run_schemes.sh
```

## Direct Runner

Use the root runner for custom degrees and sample counts:

```bash
DEGREES=5,10,15 CYCTIMES=5 MSG_NUM=5 MSG_BITS=32 ./run_schemes.sh
```

Run selected schemes directly:

```bash
build/benchmarks/schemes/scheme_group_tcc25 --degree 5 --cyctimes 1
build/benchmarks/schemes/scheme_rlwe_cz --degree 5 --cyctimes 1
```

Options:

- `DEGREES`: comma-separated degrees, default `5,10,15`.
- `CYCTIMES`: timing samples per phase, default `10`.
- `MSG_NUM`: input count, default `5`.
- `MSG_BITS`: random input bit length, default `32`.
- `SEED`: deterministic benchmark seed, default `20260615`.

Additional paths can also be overridden by environment variables:

- `BUILD_DIR`
- `OUT_DIR`

## Output Schema

The runner writes `benchmarks/results/schemes/scheme_timing.csv`:

```text
scheme,degree,seed,Setup_ms,SetupVhss_ms,SetupExtra_ms,Gen_ms,ProbGen_ms,Compute0_ms,Compute0Vhss_ms,Compute0Extra_ms,Compute1_ms,Compute1Vhss_ms,Compute1Extra_ms,Verify_ms,Decode_ms
```
