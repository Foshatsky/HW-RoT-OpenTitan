#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
YOSYS="$(cd "$SCRIPT_DIR/../.." && pwd)/tools/bin/yosys"

"$YOSYS" -p "
    read_verilog $SCRIPT_DIR/ref.v
    rename ref_add ref
    read_verilog $SCRIPT_DIR/impl.v
    rename impl_add impl
    miter -equiv -flatten ref impl miter
    sat -verify -prove-asserts miter
" 2>&1
