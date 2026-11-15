#!/bin/bash
# Experiment 3.07: Formal Property Verification (FPV)
# Uses yosys SAT solver to prove assertions in Verilog modules

YOSYS="../../tools/bin/yosys"
LOG="output.log"

echo "========================================"
echo "  Experiment 3.07: Formal Property Verification (FPV)"
echo "========================================"

run_fpv() {
    local module=$1
    local file=$2
    echo "" | tee -a $LOG
    echo "--- Proving assertions in $file ($module) ---" | tee -a $LOG

    $YOSYS -p "
        read_verilog -formal $file
        hierarchy -top $module
        proc; opt; async2sync
        sat -prove-asserts -verify -tempinduct -seq 2 $module
    " 2>&1 | tee -a $LOG

    local rc=${PIPESTATUS[0]}
    if [ $rc -eq 0 ]; then
        echo ">>> RESULT: $module - ALL ASSERTIONS PROVEN <<<"
    else
        echo ">>> RESULT: $module - VERIFICATION FAILED <<<"
    fi
    return $rc
}

# Clean log
> $LOG

# Run FPV on FSM module (3-state FSM with transition assertions)
run_fpv "fsm_props" "fsm_props.v"

# Run FPV on safe counter module (mod-3 counter with wrap assertions)
run_fpv "safe_counter" "safe_counter.v"

echo "" | tee -a $LOG
echo "========================================" | tee -a $LOG
echo "  Experiment complete. See $LOG for details." | tee -a $LOG
echo "========================================" | tee -a $LOG
