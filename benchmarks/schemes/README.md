# Scheme Benchmarks

Scheme benchmarks measure setup, input generation, server computation,
verification, and decode costs for each implemented scheme/backend pair.

## Targets

The runner covers these target names:

- `group-hss`
- `group-vhss`
- `group-ot`
- `group-dc`
- `group-tcc25`
- `rlwe-hss`
- `rlwe-vhss`
- `rlwe-ot`
- `rlwe-dc`
- `rlwe-cz`

## CMake Entry Points

```bash
# Build and run degree 5.
cmake --build build --target scheme_benchmarks_p5

# Build and run the runner default degrees.
cmake --build build --target scheme_benchmarks
```

## Direct Runner

Use the direct runner for custom degrees, sample counts, and target selection:

```bash
bash benchmarks/schemes/run_scheme_benchmarks.sh \
  --build-dir build \
  --degrees 5,10,15 \
  --cyctimes 5 \
  --msg-num 5 \
  --msg-bits 32
```

Run only selected schemes:

```bash
bash benchmarks/schemes/run_scheme_benchmarks.sh \
  --build-dir build \
  --targets group-tcc25,rlwe-cz \
  --degrees 5 \
  --cyctimes 1
```

Options:

- `--build-dir DIR`: CMake build directory, default `build`.
- `--out-dir DIR`: output directory, default `benchmarks/results/schemes`.
- `--targets LIST`: comma-separated or space-separated target names.
- `--degrees LIST`: comma-separated degrees, default `5,10,15`.
- `--cyctimes N` / `--samples N`: timing samples per phase.
- `--msg-num N`: input count.
- `--msg-bits N`: random input bit length.

The same defaults can be overridden by environment variables:

- `SCHEME_DEGREES`
- `SCHEME_CYCTIMES`
- `SCHEME_MSG_NUM`
- `SCHEME_MSG_BITS`

## Output Schema

The runner writes `benchmarks/results/schemes/scheme_timing.csv`:

```text
backend,scheme,target,degree,setup_ms,probgen_ms,compute0_ms,compute1_ms,eval_ms,verify_ms,decode_ms,correctness,log_path
```

Each run also writes a per-scheme/per-degree log under
`benchmarks/results/schemes/logs/`, for example `group-tcc25_d5.log`.
