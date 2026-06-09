# Correctness Checks

This directory is for correctness checks, not performance measurement.  The P5
checks run the degree-5 polynomial on the group-backend OT and DC paths, then
compare the decoded protocol result against the native computation.

```sh
cmake --build build --target p5_ot_group_acc p5_dc_group_acc
ctest --test-dir build -R p5 --output-on-failure
ctest --test-dir build -L fast --output-on-failure
```

Each executable returns nonzero when verification fails or the decoded result
does not match the native result.

Longer full-protocol correctness checks should also live here, but should be
registered with CTest labels such as `slow` or `full`.  The default assistant
regression path is the `fast`/`p5` subset.
