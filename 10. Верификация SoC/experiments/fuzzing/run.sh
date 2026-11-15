#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"

VERILATOR=/home/cdcat/tools/verilator-4.210/bin/verilator

echo "=== 1. Verilating tiny_alu ==="
$VERILATOR --cc tiny_alu.v --exe fuzz_driver.cpp

echo ""
echo "=== 2. Building ==="
make -C obj_dir -f Vtiny_alu.mk

echo ""
echo "=== 3. Running fuzzer ==="
./obj_dir/Vtiny_alu 2>&1 | tee output.log
