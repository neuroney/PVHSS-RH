# Correctness Checks

This directory contains correctness checks rather than performance benchmarks.
The fast P5 checks compare decoded scheme outputs against native plaintext
evaluation and fail on verification or decoding mismatch.

## Fast Checks

Run the registered fast set:

```bash
cmake --build build --target check_correctness_fast
```

Equivalent CTest invocation after the executables have been built:

```bash
ctest --test-dir build -L fast --output-on-failure
```

## P5 Coverage

Current P5 checks:

- `p5_ot_group_acc`
- `p5_dc_group_acc`
- `p5_tcc25_group_acc`
- `p5_vhss_rlwe_acc`
- `p5_ot_rlwe_acc`
- `p5_dc_rlwe_acc`

Each executable returns nonzero when verification fails or the decoded result
does not match the native computation.  Longer or heavier checks should be
registered with CTest labels such as `slow` or `full`.
