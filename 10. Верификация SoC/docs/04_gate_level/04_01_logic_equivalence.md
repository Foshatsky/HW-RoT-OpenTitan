### МЕТОД: Логическая эквивалентность RTL и вентильного нетлиста (Logic Equivalence Checking, LEC)
### КАТЕГОРИЯ: 4. Верификация на уровне вентилей
### ФОРМАЛЬНОСТЬ: формальный

### СУТЬ МЕТОДА:
Logic Equivalence Checking (LEC) — это формальный метод верификации, который сравнивает исходное RTL-описание цифровой схемы с её синтезированным вентильным нетлистом (gate-level netlist). После логического синтеза, выполняемого инструментом (Synopsys DC, [Yosys](../tools_instructions/yosys.md), Genus), RTL-код преобразуется в структуру из логических вентилей (AND, OR, XOR, NOT, etc.), триггеров и соединительных проводов. Цель LEC — доказать, что синтезированный нетлист функционально идентичен исходному RTL: для любой комбинации входов оба описания дают одинаковые выходы.

Метод является формальным (основан на SAT-решателях, доказывающих выполнимость булевых формул) и обеспечивает 100% проверку входного пространства, в отличие от симуляции, которая проверяет лишь конечное число векторов. LEC критически важен в маршруте проектирования процессоров, так как синтезаторы могут вносить логические трансформации (сокращение цепей, ретиминг, перекодирование состояний FSM, изменение структуры деревьев сумматоров), и необходимо гарантировать, что функциональность не изменилась. Типичный маршрут: RTL -> синтез -> gate-level нетлист -> LEC -> физическое проектирование (размещение, трассировка).

### ИСТОРИЧЕСКАЯ СПРАВКА:
Методология LEC стала промышленно применяться в конце 1990-х — начале 2000-х с появлением инструментов Synopsys Formality (1999), Cadence Conformal LEC (позже Cadence Logic Equivalence Checker, результат поглощения Verplex Systems в 2003). В open-source Yosys с 2014 года поддерживает miter-based equivalence checking через команды `miter` и `sat`. Ключевые теоретические работы: Brand, D. "Verification of large synthesized designs" (ICCAD 1993); Kuehlmann, A. "Equivalence Checking" (глава в "EDA for IC Implementation, Circuit Design, and Process Technology", 2006).

### ПОДРОБНОЕ ОПИСАНИЕ МЕТОДА И ЭКСПЕРИМЕНТА:
В приведенном ниже практическом примере 4-битный сумматор ref_add синтезируется через `synth -top ref_add` в Yosys, в результате чего получается синтезированный нетлист (synth_ref.v) из 20 логических элементов (AND, ANDNOT, NAND, NOR, OR, ORNOT, XNOR, XOR). Затем исходный RTL и синтезированный нетлист сравниваются через miter + SAT. Исходный RTL и нетлист имеют одинаковые порты (a[3:0], b[3:0], sum[4:0]), что позволяет построить корректную miter-схему.

Shell-скрипт выполняет два последовательных запуска Yosys:
1. Синтез: `read_verilog ref.v -> synth -top ref_add -> write_verilog synth_ref.v`. Yosys преобразует поведенческий assign sum = a + b в библиотечно-независимый нетлист на примитивных гейтах (через отображение в стандартную библиотеку).
2. Проверка эквивалентности: загружаются ref.v (переименовывается в ref_orig) и synth_ref.v (переименовывается в ref_synth), строится miter-схема (все выходы сравниваются через XOR), SAT-решатель ищет нарушение. Результат: «SAT proof finished - no model found: SUCCESS!» — эквивалентность доказана.

Miter-схема объединяет оба модуля на общие входы, а выходные сигналы подаёт на элемент XOR. Yosys автоматически создаёт assert, который должен быть всегда ложным для эквивалентных модулей.

Код сумматора преобразуется в логическое представление следующего вида:
```
 ...
  assign _00_ = b[3] & a[3];
  assign _01_ = b[3] ^ a[3];
  assign _02_ = ~(b[2] & a[2]);
  assign _03_ = _01_ & ~(_02_);
  assign _04_ = _03_ | _00_;
  …
```
То есть, высокоуровневая операция сложения была заменена на примитивные логические гейты.

### ЛИСТИНГИ (код, скрипты):

**run.sh — синтез и проверка эквивалентности:**
```bash
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
```

### ЗАПУСК И ВЫВОД:
```
--- Step 1: Synthesis ---
...
2.22. Executing ABC pass (technology mapping using ABC).
ABC RESULTS:               AND cells:        3
ABC RESULTS:            ANDNOT cells:        4
ABC RESULTS:              NAND cells:        2
ABC RESULTS:               NOR cells:        1
ABC RESULTS:                OR cells:        2
ABC RESULTS:             ORNOT cells:        1
ABC RESULTS:              XNOR cells:        1
ABC RESULTS:               XOR cells:        6
ABC RESULTS:        internal signals:       15
...
Number of cells:                 20
  $_ANDNOT_                       4
  $_AND_                          3
  $_NAND_                         2
  ...

--- Step 2: Equivalence check ---
...
3. Executing MITER pass (creating miter circuit).
Creating miter cell "miter" with gold cell "ref_orig" and gate cell "ref_synth".
...
4. Executing SAT pass (solving SAT problems in the circuit).
Solving problem with 205 variables and 528 clauses..
SAT proof finished - no model found: SUCCESS!
```

### АНАЛИЗ РЕЗУЛЬТАТОВ:
SAT proof завершился успешно — ни одной модели (контрпримера), нарушающей эквивалентность, не найдено. Это формально доказывает, что синтезированный Yosys нетлист из 20 гейтов (SDFF, AND, OR, XOR, NAND, NOR и их комбинаций) реализует в точности ту же логическую функцию, что и исходная поведенческая строка `assign sum = a + b`. Ни один из 205 битовых переменных SAT-модели не указал на расхождение. Эксперимент подтверждает корректность синтезатора Yosys для данного простого модуля.

### ВЫВОД ПО МЕТОДУ:
Logic Equivalence Checking — обязательный шаг в любом промышленном маршруте проектирования ASIC/FPGA. Он гарантирует, что синтез не внёс функциональных изменений в дизайн. Метод формально полон (100% покрытие всех возможных состояний комбинационной логики) и не требует написания тестов. Сложность реализации низкая для модулей размером до сотен тысяч гейтов, но для целых процессорных ядер (миллионы гейтов) используются коммерческие LEC-инструменты, поддерживающие иерархическое сравнение и обработку сложных структур (datapath, память). Yosys LEC подходит для верификации отдельных блоков и исследовательских проектов с открытым кодом (RISC-V cores: Picorv32, Serv, SweRV).

### ДОПОЛНИТЕЛЬНЫЕ ИСТОЧНИКИ:
- [Yosys](../tools_instructions/yosys.md)
- Synopsys Formality User Guide
- Cadence Conformal LEC User Guide
