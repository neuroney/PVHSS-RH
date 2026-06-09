#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
OUT_DIR="benchmarks/results/protocols"
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

RUNNER="$BUILD_DIR/benchmarks/protocols/protocol_benchmark_runner"

if [[ ! -x "$RUNNER" ]]; then
    echo "Missing protocol benchmark runner: $RUNNER" >&2
    exit 1
fi

"$RUNNER" --build-dir "$BUILD_DIR" --out-dir "$OUT_DIR" "${ARGS[@]}"
