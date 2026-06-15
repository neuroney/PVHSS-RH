#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/build}"
OUT_DIR="${OUT_DIR:-$SCRIPT_DIR/benchmarks/results/schemes}"
DEGREES="${DEGREES:-5,10,15}"
CYCTIMES="${CYCTIMES:-10}"
MSG_NUM="${MSG_NUM:-5}"
MSG_BITS="${MSG_BITS:-32}"
SEED="${SEED:-20260615}"

SCHEMES=(
    scheme_group_hss
    scheme_group_vhss
    scheme_group_ot
    scheme_group_dc
    scheme_group_tcc25
    scheme_rlwe_cz
    scheme_rlwe_hss
    scheme_rlwe_vhss
    scheme_rlwe_ot
    scheme_rlwe_dc
)

BENCH_DIR="$BUILD_DIR/benchmarks/schemes"

echo "===== Scheme Benchmarks ====="
echo "Degrees:  $DEGREES"
echo "Cyctimes: $CYCTIMES"
echo "Msg num:  $MSG_NUM"
echo "Msg bits: $MSG_BITS"
echo "Seed:     $SEED"
echo "Output:   $OUT_DIR/scheme_timing.csv"
echo

mkdir -p "$OUT_DIR"
OUT_FILE="$OUT_DIR/scheme_timing.csv"

# Extract a single phase's mean_ms from the output.
# Usage: extract_phase "Setup" "$output" → "12.345" or ""
extract_phase() {
    local phase="$1"
    local text="$2"
    echo "$text" | grep -oP "^${phase}:\s+\K[0-9.e+\-]+(?=\s*ms)" || true
}

# CSV header — all possible phases
echo "scheme,degree,seed,Setup_ms,SetupVhss_ms,SetupExtra_ms,Gen_ms,ProbGen_ms,Compute0_ms,Compute0Vhss_ms,Compute0Extra_ms,Compute1_ms,Compute1Vhss_ms,Compute1Extra_ms,Verify_ms,Decode_ms" > "$OUT_FILE"

IFS=',' read -r -a DEGREE_ARRAY <<< "$DEGREES"

for degree in "${DEGREE_ARRAY[@]}"; do
    degree=$(echo "$degree" | xargs)
    echo "--- degree=$degree ---"
    for scheme in "${SCHEMES[@]}"; do
        bin="$BENCH_DIR/$scheme"
        if [[ ! -x "$bin" ]]; then
            echo "  SKIP $scheme (binary not found)" >&2
            continue
        fi
        echo -n "  $scheme ... "

        output=$("$bin" \
            --degree   "$degree" \
            --cyctimes "$CYCTIMES" \
            --msg-num  "$MSG_NUM" \
            --msg-bits "$MSG_BITS" \
            --seed     "$SEED" 2>&1)

        s=$(extract_phase "Setup"    "$output")
        s_vhss=$(extract_phase "SetupVhss" "$output")
        s_extra=$(extract_phase "SetupExtra" "$output")
        gen=$(extract_phase "Gen" "$output")
        p=$(extract_phase "ProbGen"  "$output")
        c0=$(extract_phase "Compute0" "$output")
        c0_vhss=$(extract_phase "Compute0Vhss" "$output")
        c0_extra=$(extract_phase "Compute0Extra" "$output")
        c1=$(extract_phase "Compute1" "$output")
        c1_vhss=$(extract_phase "Compute1Vhss" "$output")
        c1_extra=$(extract_phase "Compute1Extra" "$output")
        v=$(extract_phase "Verify"   "$output")
        d=$(extract_phase "Decode"   "$output")

        echo "$scheme,$degree,$SEED,$s,$s_vhss,$s_extra,$gen,$p,$c0,$c0_vhss,$c0_extra,$c1,$c1_vhss,$c1_extra,$v,$d" >> "$OUT_FILE"
        echo "done"
    done
done

echo
echo "Done: $(($(wc -l < "$OUT_FILE") - 1)) rows written to $OUT_FILE"
