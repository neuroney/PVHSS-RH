#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/build}"
OUT_DIR="${OUT_DIR:-$SCRIPT_DIR/benchmarks/results/pir}"
DB_SIZES="${DB_SIZES:-1024}"
CYCTIMES="${CYCTIMES:-1}"
RECORD_BITS="${RECORD_BITS:-16}"
SEED="${SEED:-20260615}"
TAMPER="${TAMPER:-1}"
RUN_GROUP_DC="${RUN_GROUP_DC:-0}"
RUN_GROUP_OT="${RUN_GROUP_OT:-1}"
RUN_CDPIR="${RUN_CDPIR:-1}"
CDPIR_BLOCK_SIZE="${CDPIR_BLOCK_SIZE:-$(((RECORD_BITS + 7) / 8))}"
CDPIR_PRIME="${CDPIR_PRIME:-52435875175126190479447740508185965837690552500527637822603658699938581184513}"

GROUP_BIN="$BUILD_DIR/benchmarks/pir/pir_group_dc"
GROUP_OT_BIN="$BUILD_DIR/benchmarks/pir/pir_group_ot"
CDPIR_BIN="$BUILD_DIR/benchmarks/pir/cdpir"

enabled() {
    [[ "$1" == "1" || "$1" == "true" || "$1" == "yes" ]]
}

if enabled "$RUN_GROUP_DC" && [[ ! -x "$GROUP_BIN" ]]; then
    echo "Missing benchmark binary: $GROUP_BIN" >&2
    echo "Build first with: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build" >&2
    exit 1
fi

if enabled "$RUN_GROUP_OT" && [[ ! -x "$GROUP_OT_BIN" ]]; then
    echo "Missing benchmark binary: $GROUP_OT_BIN" >&2
    echo "Build first with: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build" >&2
    exit 1
fi

if enabled "$RUN_CDPIR" && [[ ! -x "$CDPIR_BIN" ]]; then
    echo "Missing benchmark binary: $CDPIR_BIN" >&2
    echo "Build first with: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
GROUP_OUT_FILE="$OUT_DIR/pir_group_dc_timing.csv"
GROUP_OT_OUT_FILE="$OUT_DIR/pir_group_ot_timing.csv"
CDPIR_OUT_FILE="$OUT_DIR/cdpir_timing.csv"
COMPARE_OUT_FILE="$OUT_DIR/pir_comparison_timing.csv"
GROUP_PIR_HEADER="scheme,db_size,logn,record_bits,index,seed,Setup_ms,SetupVhss_ms,SetupExtra_ms,Gen_ms,Query_ms,Answer0_ms,Answer0Eval_ms,Answer0Proof_ms,Answer1_ms,Answer1Eval_ms,Answer1Proof_ms,Verify_ms,Decode_ms,Correctness,TamperRejected,QueryBitsPerServer,Answer0Bits,Answer1Bits,TotalOnlineBits,FormulaQueryBitsPerServer,FormulaAnswerBitsPerServer,FormulaTotalBits"

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

sum_ms() {
    local values="$*"
    awk -v values="$values" 'BEGIN {
        total = 0
        n = split(values, parts, " ")
        for (i = 1; i <= n; ++i) total += parts[i]
        print total
    }'
}

run_group_pir() {
    local scheme="$1"
    local bin="$2"
    local out_file="$3"
    local db_size="$4"

    echo -n "  $scheme db_size=$db_size ... "

    local args=(
        --db-size "$db_size"
        --cyctimes "$CYCTIMES"
        --record-bits "$RECORD_BITS"
        --seed "$SEED"
    )
    if enabled "$TAMPER"; then
        args+=(--tamper)
    fi

    local output
    output=$("$bin" "${args[@]}" 2>&1)

    local setup setup_vhss setup_extra gen query
    local answer0 answer0_eval answer0_proof
    local answer1 answer1_eval answer1_proof
    local verify decode
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

    local logn record_bits index correctness tamper_rejected
    local query_bits answer0_bits answer1_bits total_bits
    local formula_query_bits formula_answer_bits formula_total_bits
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

    echo "$scheme,$db_size,$logn,$record_bits,$index,$SEED,$setup,$setup_vhss,$setup_extra,$gen,$query,$answer0,$answer0_eval,$answer0_proof,$answer1,$answer1_eval,$answer1_proof,$verify,$decode,$correctness,$tamper_rejected,$query_bits,$answer0_bits,$answer1_bits,$total_bits,$formula_query_bits,$formula_answer_bits,$formula_total_bits" >> "$out_file"

    local offline answer_total verify_decode online
    offline=$(sum_ms "$setup" "$gen")
    answer_total=$(sum_ms "$answer0" "$answer1")
    verify_decode=$(sum_ms "$verify" "$decode")
    online=$(sum_ms "$query" "$answer0" "$answer1" "$verify" "$decode")
    echo "$scheme,$db_size,$record_bits,$SEED,$setup,$offline,$query,$answer_total,$verify_decode,$online,$total_bits,$correctness,$tamper_rejected" >> "$COMPARE_OUT_FILE"
    echo "done"
}

if enabled "$RUN_GROUP_DC"; then
    echo "$GROUP_PIR_HEADER" > "$GROUP_OUT_FILE"
fi

if enabled "$RUN_GROUP_OT"; then
    echo "$GROUP_PIR_HEADER" > "$GROUP_OT_OUT_FILE"
fi

if enabled "$RUN_CDPIR"; then
    echo "scheme,db_size,logn,record_bits,block_size,index,seed,Setup_ms,Gen_ms,Query_ms,Answer0_ms,Answer1_ms,Verify_ms,Decode_ms,Correctness,TamperRejected,QueryBitsPerServer,Answer0Bits,Answer1Bits,TotalOnlineBits,FormulaQueryBitsPerServer,FormulaAnswerBitsPerServer,FormulaTotalBits,FieldBits,FieldBytes,MerkleDepth,PaddedItems" > "$CDPIR_OUT_FILE"
fi

echo "scheme,db_size,record_bits,seed,Setup_ms,Offline_ms,Query_ms,AnswerTotal_ms,VerifyDecode_ms,Online_ms,TotalOnlineBits,Correctness,TamperRejected" > "$COMPARE_OUT_FILE"

echo "===== PIR Benchmarks ====="
echo "DB sizes:    $DB_SIZES"
echo "Cyctimes:    $CYCTIMES"
echo "Record bits: $RECORD_BITS"
echo "Seed:        $SEED"
echo "Tamper:      $TAMPER"
echo "Run GroupDC: $RUN_GROUP_DC"
echo "Run GroupOT: $RUN_GROUP_OT"
echo "Run cd-PIR:  $RUN_CDPIR"
echo "cd-PIR block size: $CDPIR_BLOCK_SIZE bytes"
echo "cd-PIR prime:      $CDPIR_PRIME"
echo "Outputs:"
enabled "$RUN_GROUP_DC" && echo "  $GROUP_OUT_FILE"
enabled "$RUN_GROUP_OT" && echo "  $GROUP_OT_OUT_FILE"
enabled "$RUN_CDPIR" && echo "  $CDPIR_OUT_FILE"
echo "  $COMPARE_OUT_FILE"
echo

IFS=',' read -r -a SIZE_ARRAY <<< "$DB_SIZES"

for db_size in "${SIZE_ARRAY[@]}"; do
    db_size="$(echo "$db_size" | xargs)"

    if enabled "$RUN_GROUP_DC"; then
        run_group_pir "pir_group_dc" "$GROUP_BIN" "$GROUP_OUT_FILE" "$db_size"
    fi

    if enabled "$RUN_GROUP_OT"; then
        run_group_pir "pir_group_ot" "$GROUP_OT_BIN" "$GROUP_OT_OUT_FILE" "$db_size"
    fi

    if enabled "$RUN_CDPIR"; then
        echo -n "  cdpir db_size=$db_size ... "

        args=(
            --items "$db_size"
            --block-size "$CDPIR_BLOCK_SIZE"
            --cyctimes "$CYCTIMES"
            --seed "$SEED"
            --prime "$CDPIR_PRIME"
        )
        if enabled "$TAMPER"; then
            args+=(--cheat)
        fi

        output=$("$CDPIR_BIN" "${args[@]}" 2>&1)

        setup=$(extract_phase "Setup" "$output")
        gen=$(extract_phase "Gen" "$output")
        query=$(extract_phase "Query" "$output")
        answer0=$(extract_phase "Answer0" "$output")
        answer1=$(extract_phase "Answer1" "$output")
        verify=$(extract_phase "Verify" "$output")
        decode=$(extract_phase "Decode" "$output")

        logn=$(extract_field "LogN" "$output")
        record_bits=$(extract_field "RecordBits" "$output")
        block_size=$(extract_field "BlockSizeBytes" "$output")
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
        field_bits=$(extract_field "FieldBits" "$output")
        field_bytes=$(extract_field "FieldBytes" "$output")
        merkle_depth=$(extract_field "MerkleDepth" "$output")
        padded_items=$(extract_field "PaddedItems" "$output")

        echo "cdpir,$db_size,$logn,$record_bits,$block_size,$index,$SEED,$setup,$gen,$query,$answer0,$answer1,$verify,$decode,$correctness,$tamper_rejected,$query_bits,$answer0_bits,$answer1_bits,$total_bits,$formula_query_bits,$formula_answer_bits,$formula_total_bits,$field_bits,$field_bytes,$merkle_depth,$padded_items" >> "$CDPIR_OUT_FILE"

        offline=$(sum_ms "$setup" "$gen")
        answer_total=$(sum_ms "$answer0" "$answer1")
        verify_decode=$(sum_ms "$verify" "$decode")
        online=$(sum_ms "$query" "$answer0" "$answer1" "$verify" "$decode")
        echo "cdpir,$db_size,$record_bits,$SEED,$setup,$offline,$query,$answer_total,$verify_decode,$online,$total_bits,$correctness,$tamper_rejected" >> "$COMPARE_OUT_FILE"
        echo "done"
    fi
done

echo
if enabled "$RUN_GROUP_DC"; then
    echo "Group-DC rows: $(($(wc -l < "$GROUP_OUT_FILE") - 1)) written to $GROUP_OUT_FILE"
fi
if enabled "$RUN_GROUP_OT"; then
    echo "Group-OT rows: $(($(wc -l < "$GROUP_OT_OUT_FILE") - 1)) written to $GROUP_OT_OUT_FILE"
fi
if enabled "$RUN_CDPIR"; then
    echo "cd-PIR rows:   $(($(wc -l < "$CDPIR_OUT_FILE") - 1)) written to $CDPIR_OUT_FILE"
fi
echo "Compare rows: $(($(wc -l < "$COMPARE_OUT_FILE") - 1)) written to $COMPARE_OUT_FILE"
