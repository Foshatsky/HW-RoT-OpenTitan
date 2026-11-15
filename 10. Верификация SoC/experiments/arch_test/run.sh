#!/bin/bash
set -e
DIR="$(cd "$(dirname "$0")" && pwd)"
PROJ="$(cd "$DIR/../.." && pwd)"

ASM="$DIR/test_sum.s"
ELF="$DIR/test_sum.elf"
LOG="$DIR/output.log"

echo "=== Compiling RISC-V assembly ==="
riscv64-linux-gnu-gcc -nostdlib -nostartfiles -fno-pic -Wl,-no-pie \
    -Wl,-Ttext-segment=0x80000000 -Wl,-e,_start \
    -o "$ELF" "$ASM"
echo "Compiled: $ELF"

echo ""
echo "=== Running Spike (ISA simulator) with instruction log ==="
"$PROJ/tools/bin/spike" -l --isa=RV64IMAFDC "$ELF" 2>&1 | tee "$LOG"

echo ""
echo "=== Test result ==="
RESULT=$(grep "tohost" "$LOG" | grep -oP 'tohost = \K\d+')
echo "Sum 1..10 = $RESULT (expected: 55)"
if [ "$RESULT" = "55" ]; then
    echo "TEST PASSED"
else
    echo "TEST FAILED"
    exit 1
fi
