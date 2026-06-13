#!/bin/bash
# Parallel run of all 4 RLWE schemes with degree 5,10,15
# Each test gets 600s timeout, runs in background, results saved per-file.

BUILD_DIR="/home/jinyuan/CodeSpace/PVHSS-RH/build"
BENCH_DIR="$BUILD_DIR/benchmarks/schemes"
OUT_DIR="/home/jinyuan/CodeSpace/PVHSS-RH/benchmarks/results/schemes/logs"
mkdir -p "$OUT_DIR"

schemes="hss vhss ot dc"
degrees="5 10 15"
TIMEOUT=600

echo "Starting all RLWE benchmarks at $(date)"
echo "=============================================="

pids=""
for s in $schemes; do
    bin="$BENCH_DIR/scheme_rlwe_${s}_rms_bench"
    for d in $degrees; do
        log="$OUT_DIR/rlwe-${s}_d${d}.log"
        echo "  Launch: rlwe-$s d=$d  ->  $log"
        timeout $TIMEOUT "$bin" --degree $d --cyctimes 1 --msg-num 5 > "$log" 2>&1 &
        pids="$pids $!"
    done
done

echo "=============================================="
echo "All $(echo $pids | wc -w) jobs launched. Waiting..."
echo ""

# Wait for all
for pid in $pids; do
    wait $pid
done

echo ""
echo "=============================================="
echo "All done at $(date)"
echo ""
echo "=== CORRECTNESS SUMMARY ==="
echo ""

for s in $schemes; do
    for d in $degrees; do
        log="$OUT_DIR/rlwe-${s}_d${d}.log"
        if [ -f "$log" ]; then
            correct=$(grep -oP 'Correctness:\s*\K\w+' "$log" 2>/dev/null || echo "?")
            verify=$(grep -oP 'Verification:\s*\K\w+' "$log" 2>/dev/null || echo "-")
            if [ "$verify" != "-" ]; then
                echo "  RLWE ${s^^}  d=$d  | Verify: $verify  | Correct: $correct"
            else
                echo "  RLWE ${s^^}  d=$d  | Correct: $correct"
            fi
        else
            echo "  RLWE ${s^^}  d=$d  | (no log file)"
        fi
    done
    echo ""
done
