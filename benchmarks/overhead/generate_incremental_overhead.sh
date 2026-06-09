#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
PROTOCOL_COMPONENTS="benchmarks/results/protocols/logs/protocol_components.csv"
OUT_DIR="benchmarks/results/overhead"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --protocol-components)
            PROTOCOL_COMPONENTS="$2"
            shift 2
            ;;
        --out-dir)
            OUT_DIR="$2"
            shift 2
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 2
            ;;
    esac
done

GENERATOR="$BUILD_DIR/benchmarks/overhead/incremental_timing_generator"

if [[ ! -x "$GENERATOR" ]]; then
    echo "Missing incremental timing generator: $GENERATOR" >&2
    exit 1
fi

"$GENERATOR" --protocol-components "$PROTOCOL_COMPONENTS" --out-dir "$OUT_DIR"
