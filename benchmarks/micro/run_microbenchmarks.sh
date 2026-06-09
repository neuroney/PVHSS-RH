#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
OUT_DIR="benchmarks/results/micro"
ARGS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --out-dir)
            OUT_DIR="$2"
            shift 2
            ;;
        *)
            ARGS+=("$1")
            shift
            ;;
    esac
done

MICROBENCH="$BUILD_DIR/benchmarks/micro/microbench"
VHSS_RLWE_BENCH="$BUILD_DIR/benchmarks/micro/microbench_vhss_rlwe"
OUT_FILE="$OUT_DIR/micro_timing.csv"

if [[ ! -x "$MICROBENCH" ]]; then
    echo "Missing microbenchmark binary: $MICROBENCH" >&2
    exit 1
fi
if [[ ! -x "$VHSS_RLWE_BENCH" ]]; then
    echo "Missing microbenchmark binary: $VHSS_RLWE_BENCH" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
{
    "$MICROBENCH" --compact "${ARGS[@]}"
    "$VHSS_RLWE_BENCH" --compact --no-header "${ARGS[@]}"
} > "$OUT_FILE"

rows=$(($(wc -l < "$OUT_FILE") - 1))
echo "Wrote $rows micro timing rows to $OUT_FILE"
