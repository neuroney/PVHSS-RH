#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/build}"
OUT_DIR="${OUT_DIR:-$SCRIPT_DIR/benchmarks/results/micro}"
SAMPLES="${SAMPLES:-2}"
ITERS="${ITERS:-50}"

MICROBENCH="$BUILD_DIR/benchmarks/micro/microbench"

if [[ ! -x "$MICROBENCH" ]]; then
    echo "Error: microbench binary not found at $MICROBENCH" >&2
    echo "Run: cmake --build $BUILD_DIR" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"

echo "===== Micro Benchmarks ====="
echo "Binary:  $MICROBENCH"
echo "Output:  $OUT_DIR/micro_timing.csv"

"$MICROBENCH" --compact \
    --samples "$SAMPLES" \
    --iters   "$ITERS" \
    > "$OUT_DIR/micro_timing.csv"

rows=$(($(wc -l < "$OUT_DIR/micro_timing.csv") - 1))
echo "Done: $rows rows written to $OUT_DIR/micro_timing.csv"
