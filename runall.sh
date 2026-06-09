#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"

if [[ $# -ne 0 ]]; then
    echo "Usage: $0"
    echo "runall.sh always builds, runs correctness checks, and generates full benchmark tables."
    exit 2
fi

echo "===== PVHSS-RH runall: full ====="
echo "Build dir: $BUILD_DIR"
echo "Build type: $BUILD_TYPE"
echo "Jobs: $JOBS"

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR" -j "$JOBS"

ctest --test-dir "$BUILD_DIR" -L fast --output-on-failure
cmake --build "$BUILD_DIR" --target benchmark_tables -j "$JOBS"

echo "===== Final benchmark tables ====="
echo "benchmarks/results/micro/micro_timing.csv"
echo "benchmarks/results/protocols/protocol_timing.csv"
echo "benchmarks/results/overhead/incremental_timing.csv"
