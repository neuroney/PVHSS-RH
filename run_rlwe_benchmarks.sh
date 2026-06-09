#!/usr/bin/env bash
# Run RLWE protocol benchmarks with real-time logging
set -euo pipefail

cd "$(dirname "$0")"

BUILD_DIR="${BUILD_DIR:-build}"
OUT_DIR="benchmarks/results/protocols"
LOG_DIR="$OUT_DIR/logs"
DEGREES="${DEGREES:-5,10,15}"
SAMPLES="${SAMPLES:-1}"
MSG_NUM="${MSG_NUM:-5}"
DYLD_PATH="${DYLD_LIBRARY_PATH:-/usr/local/lib}"

mkdir -p "$LOG_DIR"

TARGETS=(
    "rlwe-hss:protocol_rlwe_hss_bench"
    "rlwe-vhss:protocol_rlwe_vhss_bench"
    "rlwe-ot:protocol_rlwe_ot_bench"
    "rlwe-dc:protocol_rlwe_dc_bench"
    "rlwe-cz:protocol_cz_bench"
)

for entry in "${TARGETS[@]}"; do
    name="${entry%%:*}"
    binary="${entry##*:}"
    log="$LOG_DIR/$name.log"

    echo "===== [$name] started at $(date) ====="
    echo "  binary: $BUILD_DIR/benchmarks/protocols/$binary"
    echo "  log:    $log"
    echo "  args:   --msg-num $MSG_NUM --samples $SAMPLES --degrees $DEGREES"

    DYLD_LIBRARY_PATH="$DYLD_PATH" \
    "$BUILD_DIR/benchmarks/protocols/$binary" \
        --msg-num "$MSG_NUM" \
        --samples "$SAMPLES" \
        --degrees "$DEGREES" \
        > "$log" 2>&1

    rc=$?
    echo "===== [$name] finished at $(date), exit=$rc ====="
    echo "  $log: $(wc -l < "$log") lines"
    if [[ $rc -ne 0 ]]; then
        echo "===== FAILED, stopping ====="
        exit $rc
    fi
done

echo "===== All RLWE benchmarks done ====="
