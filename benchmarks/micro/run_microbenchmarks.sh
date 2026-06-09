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
OUT_FILE="$OUT_DIR/micro_timing.csv"

if [[ ! -x "$MICROBENCH" ]]; then
    echo "Missing microbenchmark binary: $MICROBENCH" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
"$MICROBENCH" --compact ${ARGS[@]+"${ARGS[@]}"} > "$OUT_FILE"

rows=$(($(wc -l < "$OUT_FILE") - 1))
echo "Wrote $rows micro timing rows to $OUT_FILE"
