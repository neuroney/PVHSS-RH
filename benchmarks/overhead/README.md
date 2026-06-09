# Incremental Timing Table

`incremental_timing_generator` generates the final OT/DC incremental timing
table.  It reads the internal protocol component CSV at
`benchmarks/results/protocols/logs/protocol_components.csv` and does not rerun
the expensive Eval benchmarks.

Run:

```sh
bash benchmarks/overhead/generate_incremental_overhead.sh --build-dir build
```

It writes:

```text
benchmarks/results/overhead/incremental_timing.csv
```

with columns:

```text
backend,scheme,parent,degree,setup_incremental_ms,gen_incremental_ms,input_incremental_ms,eval_incremental_ms,verify_incremental_ms,decode_incremental_ms
```

Use `--protocol-components` to point at a different component CSV.  When
protocol benchmarks include direct incremental rows, those rows are used
directly.  Otherwise the script falls back to subtracting the VHSS stage timing
from the scheme stage timing.  Input has no OT/DC-specific operation in the
current implementations, so its incremental value is reported as `0.000000`
when the VHSS input baseline is present.
