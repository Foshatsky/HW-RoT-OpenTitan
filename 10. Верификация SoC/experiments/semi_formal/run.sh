#!/bin/bash
# Semi-formal verification: Bounded Model Checking (BMC)
# Демонстрирует разницу между BMC (полуформальный) и индукцией (формальный)

YOSYS=../../tools/bin/yosys
DIR=../../experiments/semi_formal

echo "=================================================="
echo "ПОЛУФОРМАЛЬНАЯ ВЕРИФИКАЦИЯ"
echo "Bounded Model Checking (BMC) vs полная индукция"
echo "=================================================="
echo ""

echo "---"
echo "Модуль: bmc_fsm (5-состояний FSM с запрещённым состоянием 5-7)"
echo "Свойство 1: state всегда <= 4 (ERROR)"
echo "Свойство 2: если done=1, то state=DONE"
echo "---"
echo ""

# === 1. BMC (ограниченная глубина, без индукции) ===
echo "=== 1. BMC: проверка свойств на глубине k=5 ==="
echo "    Полуформальная: верно для k <= 5,"
echo "    НЕ гарантирует для всех k"
echo ""

$YOSYS -p "
    read_verilog -formal $DIR/bmc_fsm.v
    hierarchy -top bmc_fsm
    proc; opt; async2sync
    sat -prove-asserts -seq 5 bmc_fsm
" 2>&1 | grep -E "SAT|proved|FAIL|SUCCESS|counterexample|Assert|import|Solving|base case|induction|step"

echo ""
echo ""

# === 2. Попытка индукции (формальная) ===
echo "=== 2. Формальная: индукция (k=2, с verify) ==="
echo "    Доказывает свойство для ВСЕХ k"
echo "    Требует更强 invariants для сложных FSM"
echo ""

$YOSYS -p "
    read_verilog -formal $DIR/bmc_fsm.v
    hierarchy -top bmc_fsm
    proc; opt; async2sync
    sat -prove-asserts -verify -tempinduct -seq 2 bmc_fsm
" 2>&1 | grep -E "SAT|proved|FAIL|SUCCESS|counterexample|Assert|import|Solving|base case|induction|step"

echo ""
echo "=================================================="
echo "АНАЛИЗ РАЗЛИЧИЙ"
echo "=================================================="
echo ""
echo "BMC (полуформальная):"
echo "  - Проверяет свойства на k тактов вперёд"
echo "  - Если найдёт баг — выдаёт контрпример"
echo "  - Если не найдёт — свойство может быть"
echo "    нарушено на глубине > k"
echo "  - Быстрее, масштабируется на большие схемы"
echo ""
echo "Индукция (формальная):"
echo "  - Доказывает свойство для ВСЕХ состояний"
echo "  - Требует более сильных инвариантов"
echo "  - Может не сойтись на сложных FSM"
echo "  - Полная гарантия корректности"
echo ""
echo "Практика: сначала BMC (k=50-100) для поиска"
echo "багов, затем индукция для полного доказательства."
echo "Это и есть полуформальный подход."
echo "=================================================="
