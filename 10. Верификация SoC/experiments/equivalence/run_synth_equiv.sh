#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
YOSYS="$(cd "$SCRIPT_DIR/../.." && pwd)/tools/bin/yosys"

"$YOSYS" -p "
    read_verilog $SCRIPT_DIR/ref.v
    synth -top ref_add
    write_verilog $SCRIPT_DIR/synth_ref.v
" 2>&1

"$YOSYS" -p "
    read_verilog $SCRIPT_DIR/ref.v
    rename ref_add ref_orig
    read_verilog $SCRIPT_DIR/synth_ref.v
    rename ref_add ref_synth
    miter -equiv -flatten ref_orig ref_synth miter
    sat -verify -prove-asserts miter
" 2>&1
