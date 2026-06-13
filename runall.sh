#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

# --- Apply defaults ------------------------------------------
BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
SKIP_BUILD="${SKIP_BUILD:-0}"

MICRO_SAMPLES="${MICRO_SAMPLES:-1}"
MICRO_ITERS="${MICRO_ITERS:-1}"

SCHEME_DEGREES="${SCHEME_DEGREES:-5,10,15}"
SCHEME_CYCTIMES="${SCHEME_CYCTIMES:-5}"
SCHEME_MSG_NUM="${SCHEME_MSG_NUM:-5}"

RUN_MODE="${RUN_MODE:-full}"

RESULT_DIR="benchmarks/results"

echo "===== PVHSS-RH runall ======"
echo "Mode:     $RUN_MODE"
echo "Build:    $BUILD_DIR ($BUILD_TYPE), jobs=$JOBS"
echo "Micro:    samples=${MICRO_SAMPLES}  iters=${MICRO_ITERS}"
echo "Scheme:   degrees=$SCHEME_DEGREES  cyctimes=$SCHEME_CYCTIMES  msg_num=$SCHEME_MSG_NUM"

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
        --samples "$MICRO_SAMPLES" \
        --iters   "$MICRO_ITERS" \
        > "$RESULT_DIR/micro/micro_timing.csv"
    echo "Wrote $RESULT_DIR/micro/micro_timing.csv"
}

# --- Helper: run schemes --------------------------------------
run_scheme() {
    echo "===== Scheme benchmarks ====="
    bash benchmarks/schemes/run_scheme_benchmarks.sh \
        --build-dir "$BUILD_DIR" \
        --out-dir  "$RESULT_DIR/schemes" \
        --degrees  "$SCHEME_DEGREES" \
        --cyctimes "$SCHEME_CYCTIMES" \
        --msg-num  "$SCHEME_MSG_NUM"
}

# --- Dispatch by mode -----------------------------------------
case "$RUN_MODE" in
    full)
        run_micro
        run_scheme
        ;;
    p5)
        SCHEME_DEGREES="5"
        run_micro
        run_scheme
        ;;
    micro)
        run_micro
        ;;
    scheme)
        run_scheme
        ;;
    *)
        echo "Unknown RUN_MODE: $RUN_MODE" >&2
        exit 2
        ;;
esac

echo "===== Final results ====="
echo "micro:      $RESULT_DIR/micro/micro_timing.csv"
echo "scheme:     $RESULT_DIR/schemes/scheme_timing.csv"
