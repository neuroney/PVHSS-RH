#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"
OUT_DIR="benchmarks/results/schemes"
DEGREES="${SCHEME_DEGREES:-5,10,15}"
CYCTIMES="${SCHEME_CYCTIMES:-3}"
MSG_NUM="${SCHEME_MSG_NUM:-5}"
MSG_BITS="${SCHEME_MSG_BITS:-32}"
SELECTED=()

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
        --degrees)
            DEGREES="$2"
            shift 2
            ;;
        --cyctimes|--samples)
            CYCTIMES="$2"
            shift 2
            ;;
        --msg-num)
            MSG_NUM="$2"
            shift 2
            ;;
        --msg-bits)
            MSG_BITS="$2"
            shift 2
            ;;
        --targets)
            shift
            while [[ $# -gt 0 && "$1" != --* ]]; do
                IFS=',' read -r -a parts <<< "$1"
                for part in "${parts[@]}"; do
                    [[ -n "$part" ]] && SELECTED+=("$part")
                done
                shift
            done
            ;;
        --help)
            cat <<'USAGE'
Usage: run_scheme_benchmarks.sh [options]

Options:
  --build-dir DIR       CMake build directory (default: build)
  --out-dir DIR         Output directory (default: benchmarks/results/schemes)
  --targets LIST        Targets by name, comma-separated or space-separated
                        (default: all)
  --degrees LIST        Comma-separated degrees (default: $SCHEME_DEGREES or 5,10,15)
  --cyctimes N          Timing samples per phase (default: $SCHEME_CYCTIMES or 3)
  --samples N           Alias for --cyctimes
  --msg-num N           Input count (default: $SCHEME_MSG_NUM or 5)
  --msg-bits N          Random input bit length (default: $SCHEME_MSG_BITS or 32)
USAGE
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 2
            ;;
    esac
done

TARGETS=(
    "group-hss|group|hss|scheme_hss_rms_bench"
    "group-vhss|group|vhss|scheme_vhss_rms_bench"
    "group-ot|group|ot|scheme_ot_rms_bench"
    "group-dc|group|dc|scheme_dc_rms_bench"
    "group-tcc25|group|tcc25|scheme_tcc25_rms_bench"
    "rlwe-hss|rlwe|hss|scheme_rlwe_hss_rms_bench"
    "rlwe-vhss|rlwe|vhss|scheme_rlwe_vhss_rms_bench"
    "rlwe-ot|rlwe|ot|scheme_rlwe_ot_rms_bench"
    "rlwe-dc|rlwe|dc|scheme_rlwe_dc_rms_bench"
    "rlwe-cz|rlwe|cz|scheme_cz_rms_bench"
)

want_target() {
    local name="$1"
    if [[ ${#SELECTED[@]} -eq 0 ]]; then
        return 0
    fi
    local selected
    for selected in "${SELECTED[@]}"; do
        [[ "$selected" == "$name" ]] && return 0
    done
    return 1
}

extract_row() {
    local backend="$1"
    local scheme="$2"
    local target="$3"
    local degree="$4"
    local log_path="$5"
    awk -v backend="$backend" -v scheme="$scheme" -v target="$target" \
        -v degree="$degree" -v log_path="$log_path" '
        function ms_value(line, parts) {
            split(line, parts, ": ")
            split(parts[2], parts, " ")
            return parts[1]
        }
        /^Setup:/ { setup = ms_value($0) }
        /^ProbGen:/ { probgen = ms_value($0) }
        /^Compute0:/ { compute0 = ms_value($0) }
        /^Compute1:/ { compute1 = ms_value($0) }
        /^Verify:/ { verify = ms_value($0) }
        /^Decode: skipped/ { decode = "skipped" }
        /^Decode:/ && $0 !~ /skipped/ { decode = ms_value($0) }
        /Correctness:/ {
            correctness = $0
            sub(/^  Correctness: /, "", correctness)
        }
        END {
            eval_ms = compute0
            if (compute1 + 0 > eval_ms + 0) {
                eval_ms = compute1
            }
            print backend "," scheme "," target "," degree "," setup "," probgen "," compute0 "," compute1 "," eval_ms "," verify "," decode "," correctness "," log_path
        }
    ' "$log_path"
}

mkdir -p "$OUT_DIR/logs"
CSV="$OUT_DIR/scheme_timing.csv"
echo "backend,scheme,target,degree,setup_ms,probgen_ms,compute0_ms,compute1_ms,eval_ms,verify_ms,decode_ms,correctness,log_path" > "$CSV"

IFS=',' read -r -a DEGREE_LIST <<< "$DEGREES"
failures=0
for target_spec in "${TARGETS[@]}"; do
    IFS='|' read -r target_name backend scheme binary_name <<< "$target_spec"
    if ! want_target "$target_name"; then
        continue
    fi

    binary="$BUILD_DIR/benchmarks/schemes/$binary_name"
    if [[ ! -x "$binary" ]]; then
        echo "Missing scheme benchmark binary: $binary" >&2
        exit 1
    fi

    for degree in "${DEGREE_LIST[@]}"; do
        [[ -n "$degree" ]] || continue
        log_path="$OUT_DIR/logs/${target_name}_d${degree}.log"
        echo "Running $target_name degree=$degree"
        "$binary" \
            --msg-num "$MSG_NUM" \
            --cyctimes "$CYCTIMES" \
            --degree "$degree" \
            --msg-bits "$MSG_BITS" \
            > "$log_path"
        extract_row "$backend" "$scheme" "$target_name" "$degree" "$log_path" >> "$CSV"
        if ! grep -q "Correctness: PASS" "$log_path"; then
            failures=1
        fi
    done
done

rows=$(($(wc -l < "$CSV") - 1))
echo "Wrote $rows scheme timing rows to $CSV"

exit "$failures"
