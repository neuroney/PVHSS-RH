#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/build}"
OUT_DIR="${OUT_DIR:-$SCRIPT_DIR/benchmarks/results/pir}"
DB_SIZES="${DB_SIZES:-4,8,16}"
CYCTIMES="${CYCTIMES:-1}"
RECORD_BITS="${RECORD_BITS:-16}"
SEED="${SEED:-20260615}"
TAMPER="${TAMPER:-1}"

BIN="$BUILD_DIR/benchmarks/pir/pir_group_dc"

if [[ ! -x "$BIN" ]]; then
    echo "Missing benchmark binary: $BIN" >&2
    echo "Build first with: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
OUT_FILE="$OUT_DIR/pir_group_dc_timing.csv"

extract_phase() {
    local phase="$1"
    local text="$2"
    awk -v phase="$phase" '$1 == phase ":" { print $2; exit }' <<< "$text"
}

extract_field() {
    local field="$1"
    local text="$2"
    awk -v field="$field" '$1 == field ":" { print $2; exit }' <<< "$text"
}

echo "scheme,db_size,logn,record_bits,index,seed,Setup_ms,SetupVhss_ms,SetupExtra_ms,Gen_ms,Query_ms,Answer0_ms,Answer0Eval_ms,Answer0Proof_ms,Answer1_ms,Answer1Eval_ms,Answer1Proof_ms,Verify_ms,Decode_ms,Correctness,TamperRejected,QueryBitsPerServer,Answer0Bits,Answer1Bits,TotalOnlineBits,FormulaQueryBitsPerServer,FormulaAnswerBitsPerServer,FormulaTotalBits" > "$OUT_FILE"

echo "===== PIR Benchmarks ====="
echo "DB sizes:    $DB_SIZES"
echo "Cyctimes:    $CYCTIMES"
echo "Record bits: $RECORD_BITS"
echo "Seed:        $SEED"
echo "Tamper:      $TAMPER"
echo "Output:      $OUT_FILE"
echo

IFS=',' read -r -a SIZE_ARRAY <<< "$DB_SIZES"

for db_size in "${SIZE_ARRAY[@]}"; do
    db_size="$(echo "$db_size" | xargs)"
    echo -n "  pir_group_dc db_size=$db_size ... "

    args=(
        --db-size "$db_size"
        --cyctimes "$CYCTIMES"
        --record-bits "$RECORD_BITS"
        --seed "$SEED"
    )
    if [[ "$TAMPER" == "1" || "$TAMPER" == "true" || "$TAMPER" == "yes" ]]; then
        args+=(--tamper)
    fi

    output=$("$BIN" "${args[@]}" 2>&1)

    setup=$(extract_phase "Setup" "$output")
    setup_vhss=$(extract_phase "SetupVhss" "$output")
    setup_extra=$(extract_phase "SetupExtra" "$output")
    gen=$(extract_phase "Gen" "$output")
    query=$(extract_phase "Query" "$output")
    answer0=$(extract_phase "Answer0" "$output")
    answer0_eval=$(extract_phase "Answer0Eval" "$output")
    answer0_proof=$(extract_phase "Answer0Proof" "$output")
    answer1=$(extract_phase "Answer1" "$output")
    answer1_eval=$(extract_phase "Answer1Eval" "$output")
    answer1_proof=$(extract_phase "Answer1Proof" "$output")
    verify=$(extract_phase "Verify" "$output")
    decode=$(extract_phase "Decode" "$output")

    logn=$(extract_field "LogN" "$output")
    record_bits=$(extract_field "RecordBits" "$output")
    index=$(extract_field "Index" "$output")
    correctness=$(extract_field "Correctness" "$output")
    tamper_rejected=$(extract_field "TamperRejected" "$output")
    query_bits=$(extract_field "QueryBitsPerServer" "$output")
    answer0_bits=$(extract_field "Answer0Bits" "$output")
    answer1_bits=$(extract_field "Answer1Bits" "$output")
    total_bits=$(extract_field "TotalOnlineBits" "$output")
    formula_query_bits=$(extract_field "FormulaQueryBitsPerServer" "$output")
    formula_answer_bits=$(extract_field "FormulaAnswerBitsPerServer" "$output")
    formula_total_bits=$(extract_field "FormulaTotalBits" "$output")

    echo "pir_group_dc,$db_size,$logn,$record_bits,$index,$SEED,$setup,$setup_vhss,$setup_extra,$gen,$query,$answer0,$answer0_eval,$answer0_proof,$answer1,$answer1_eval,$answer1_proof,$verify,$decode,$correctness,$tamper_rejected,$query_bits,$answer0_bits,$answer1_bits,$total_bits,$formula_query_bits,$formula_answer_bits,$formula_total_bits" >> "$OUT_FILE"
    echo "done"
done

echo
echo "Done: $(($(wc -l < "$OUT_FILE") - 1)) rows written to $OUT_FILE"
