# Protocol Benchmarks

Protocol benchmarks measure phase costs for the implemented schemes and write a
single compact timing table.  The final table has one row per
`backend/scheme/degree` and these stage columns:

```text
backend,scheme,degree,setup_ms,gen_ms,input_ms,eval_ms,verify_ms,decode_ms
```

Build the benchmark entry points:

```sh
cmake --build build --target protocol_group_ot_bench protocol_group_dc_bench
```

Run the unified runner on a small P5 subset:

```sh
bash benchmarks/protocols/run_protocol_benchmarks.sh \
  --build-dir build \
  --targets group-ot group-dc \
  --degrees 5 \
  --samples 1
```

Run all protocol benchmarks used for paper tables:

```sh
cmake --build build --target protocol_benchmarks
```

The runner writes:

- `benchmarks/results/protocols/protocol_timing.csv`
- `benchmarks/results/protocols/logs/*.log`
- `benchmarks/results/protocols/logs/protocol_components.csv`

`protocol_timing.csv` is the presentation table.  The component CSV keeps the
normalized phase rows used to assemble the compact timing table.
