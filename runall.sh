#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

# --- Source config -------------------------------------------
CONF="./bench.conf"
if [[ -f "$CONF" ]]; then
    source "$CONF"
fi

# --- Apply defaults ------------------------------------------
BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
SKIP_BUILD="${SKIP_BUILD:-0}"

MICRO_CHEAP_SAMPLES="${MICRO_CHEAP_SAMPLES:-10}"
MICRO_CHEAP_ITERS="${MICRO_CHEAP_ITERS:-1}"
MICRO_EXPENSIVE_SAMPLES="${MICRO_EXPENSIVE_SAMPLES:-1}"
MICRO_EXPENSIVE_ITERS="${MICRO_EXPENSIVE_ITERS:-1}"

PROTO_DEGREES="${PROTO_DEGREES:-5,10,15}"
PROTO_CYCTIMES="${PROTO_CYCTIMES:-3}"
PROTO_MSG_NUM="${PROTO_MSG_NUM:-5}"

RUN_MODE="${RUN_MODE:-full}"

RESULT_DIR="benchmarks/results"

echo "===== PVHSS-RH runall ======"
echo "Config:   $CONF"
echo "Mode:     $RUN_MODE"
echo "Build:    $BUILD_DIR ($BUILD_TYPE), jobs=$JOBS"
echo "Micro:    cheap=${MICRO_CHEAP_SAMPLES}x${MICRO_CHEAP_ITERS}  expensive=${MICRO_EXPENSIVE_SAMPLES}x${MICRO_EXPENSIVE_ITERS}"
echo "Protocol: degrees=$PROTO_DEGREES  cyctimes=$PROTO_CYCTIMES  msg_num=$PROTO_MSG_NUM"

# --- Build ----------------------------------------------------
if [[ "$SKIP_BUILD" != "1" ]]; then
    cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    cmake --build "$BUILD_DIR" -j "$JOBS"
fi

# --- Helper: run micro ----------------------------------------
run_micro() {
    echo "===== Micro-benchmarks ====="
    mkdir -p "$RESULT_DIR/micro"
    "$BUILD_DIR/benchmarks/micro/microbench" --compact \
        --cheap-samples   "$MICRO_CHEAP_SAMPLES" \
        --cheap-iters     "$MICRO_CHEAP_ITERS" \
        --expensive-samples "$MICRO_EXPENSIVE_SAMPLES" \
        --expensive-iters "$MICRO_EXPENSIVE_ITERS" \
        > "$RESULT_DIR/micro/micro_timing.csv"
    echo "Wrote $RESULT_DIR/micro/micro_timing.csv"
}

# --- Helper: run protocols ------------------------------------
run_proto() {
    echo "===== Protocol benchmarks ====="
    mkdir -p "$RESULT_DIR/protocols/logs"
    "$BUILD_DIR/benchmarks/protocols/protocol_benchmark_runner" \
        --build-dir "$BUILD_DIR" \
        --out-dir  "$RESULT_DIR/protocols" \
        --degrees  "$PROTO_DEGREES" \
        --cyctimes "$PROTO_CYCTIMES" \
        --msg-num  "$PROTO_MSG_NUM"
}

# --- Dispatch by mode -----------------------------------------
case "$RUN_MODE" in
    full)
        run_micro
        run_proto
        ;;
    p5)
        PROTO_DEGREES="5"
        run_micro
        run_proto
        ;;
    micro)
        run_micro
        ;;
    proto)
        run_proto
        ;;
    *)
        echo "Unknown RUN_MODE: $RUN_MODE" >&2
        exit 2
        ;;
esac

echo "===== Final results ====="
echo "micro:      $RESULT_DIR/micro/micro_timing.csv"
echo "protocol:   $RESULT_DIR/protocols/protocol_timing.csv"
