# Верификация процессоров RISC-V

## Введение

В данном исследовании изучены популярные методы верификации систем на кристалле (SoC), в частности, микропроцессоров с акцентом на архитектуру RISC-V. Изученные методы разбиты на категории, соответствующие стадиям разработки системы — начиная с разработки концепции, требований к системе, архитектуры и заканчивая финальной проверкой готовой системы. Данные методы классифицированы по параметру формальности — упор в исследовании сделан на формальные и полуформальные методы верификации, хотя только ими оно не ограничивается.

Для каждого метода приведены:
- объяснение сути метода: что и как он проверяет;
- историческая справка;
- практический пример с запуском и анализом результатов, когда это возможно;
- вывод по методу: в каких случаях применять, сильные и слабые стороны;
- источники для углублённого изучения.

## Стадии разработки системы на кристалле

Разработка SoC — сложный многоэтапный процесс преобразования идеи в физический чип. Выделены следующие этапы:

### Этап 1: Спецификация и архитектурное проектирование
На этом этапе закладываются основные решения: выбор технологического процесса (130 нм, 32 нм, 28 нм), архитектура SoC и ядра, разделение на software и hardware.

### Этап 2: RTL-проектирование (Register-Transfer Level Design)
Создание цифрового «чертежа» микросхемы на языках описания аппаратуры (Verilog, SystemVerilog). Включает создание RTL-кода отдельных модулей и сборку из них итоговой схемы.

### Этап 3: Функциональная верификация
Динамическая верификация (запуск RTL-кода в симуляторах) с подачей тестовых воздействий и проверкой результатов, а также формальная и полуформальная верификация.

### Этап 4: Логический синтез
Переход от RTL-кода к списку логических элементов (вентилей, триггеров) из стандартной библиотеки ячеек выбранного технологического процесса. Результат — вентильный нетлист (Gate-level Netlist).

### Этап 5: Физическое проектирование
Превращение нетлиста в GDSII — чертеж для производства. Состоит из стадий: Floorplanning, Placement, Clock Tree Synthesis, Routing, Signoff & Verification, GDSII Generation.

### Этап 6: Посткремниевая валидация
Повторные проверки на полученном чипе.

В данном исследовании разделение на этапы переработано для контекста изучения методов верификации:
- **Концепция**: аналитическая проверка математических алгоритмов;
- **Требования**: верификация функциональных требований;
- **Архитектура**: проверка ISA, модели памяти, архитектурные тесты;
- **RTL**: основная верификация на RTL-уровне;
- **Gate-level**: верификация после синтеза;
- **Physical**: физическая верификация;
- **Post-silicon**: проверки на реальных чипах.

## Работа с репозиторием

### Структура

```
docs/                          # Теория по методам верификации
├── README.md                  # Этот файл: введение, таблица, источники
├── 00_concept/                # Этап концепции
│   ├── 00_intro.md            #   Введение в этап
│   └── 00_01_analytical_check.md
├── 01_requirements/           # Этап требований
│   ├── 01_intro.md
│   ├── 01_01_formal_specification.md
│   ├── 01_02_review.md
│   └── 01_03_executable_spec.md
├── 02_architecture/           # Архитектурная верификация
│   ├── 02_intro.md
│   ├── 02_01_isa_formal_sail.md
│   ├── 02_02_memory_model_herd7.md
│   └── 02_03_architectural_testing_simulation.md
├── 03_microarchitecture_rtl/  # RTL-верификация
│   ├── 03_intro.md
│   ├── 03_01_informal/        #   Неформальные методы
│   ├── 03_02_formal/          #   Формальные методы
│   └── 03_03_semi-formal/     #   Полуформальные методы
├── 04_gate_level/             # Верификация на уровне вентилей
├── 05_physical/               # Физическая верификация
└── 06_post_silicon/           # Пост-кремниевая валидация

tools_instructions/            # Инструкции по установке инструментов
  ├── README.md                #   Список всех инструментов
  ├── z3.md, yosys.md, spike.md, ...
  └── ...

experiments/                   # Код практических примеров
  ├── analytical_check/        #   Z3, сумматор/декодер
  ├── performance_model/       #   CPI-модель на Python
  ├── formal_spec/             #   Model checker на Python
  ├── arch_test/               #   Spike + test_sum.S
  ├── cocotb/                  #   cocotb + adder
  ├── fuzzing/                 #   Verilator, tiny_alu
  ├── fpv/                     #   Yosys, FSM properties
  ├── equivalence/             #   Yosys miter + synth
  ├── timing_sim/              #   Icarus, #delay
  └── ...                      #   и т.д.
```

### Как пользоваться

1. **Установить инструменты** — откройте `docs/tools_instructions/README.md`.
   Для каждого инструмента там есть команды установки (apt или сборка из
   исходников). Сначала поставьте системные зависимости (`build-essential`,
   `git`, `cmake`), затем соберите нужные инструменты.

2. **Изучать теорию** — последовательно по этапам (00 → 06). В каждом этапе
   открывайте `*_intro.md` для введения, затем файлы конкретных методов.
   Каждый метод описан по единому шаблону:
   - Суть метода, историческая справка
   - Практический пример с кодом
   - Результаты запуска и анализ
   - Вывод и источники

3. **Воспроизводить эксперименты** — для каждого метода с практическим
   примером перейдите в соответствующую директорию `experiments/<method>/`
   и выполните `bash run.sh`. Перед запуском убедитесь, что нужные
   инструменты установлены (см. `tools_instructions/`).

4. **Рекомендуемый порядок** — от простого к сложному:
   - Начните с `00_01_analytical_check` (Z3, только Python)
   - Перейдите к `00_02_performance_model` (Python, CPI-анализ)
   - Затем `01_01_formal_specification` (model checker на Python)
   - Далее `02_03_architectural_testing` (Spike, ассемблер)
   - После этого RTL-методы: `cocotb`, `fuzzing`, `fpv`, `equivalence`
   - Завершите `04_02_timing_simulation` (Icarus, задержки)

   Остальные можно изучать в произвольном порядке.

## Сводная таблица методов

| Категория | Метод | Формальность |
|---|---|---|
| **Концепция** | Аналитическая проверка алгоритмов (Z3) | Формальный |
| **Концепция** | Валидация модели производительности | Неформальный |
| **Требования** | Формальная спецификация (TLA+) | Формальный |
| **Требования** | Рецензирование (code review) | Неформальный |
| **Требования** | Исполняемая спецификация (SystemC) | Полуформальный |
| **Архитектура** | Формальная проверка ISA (Sail) | Формальный |
| **Архитектура** | Верификация модели памяти (herd7) | Формальный |
| **Архитектура** | Архитектурное тестирование (Spike) | Неформальный |
| **RTL** | Прогон реального ПО (bare-metal) | Неформальный |
| **RTL** | Контролируемо-случайное тестирование (cocotb) | Неформальный |
| **RTL** | Верификация на основе покрытия (CDV) | Неформальный |
| **RTL** | Направленное тестирование | Неформальный |
| **RTL** | Эмуляция / FPGA-прототипирование | Неформальный |
| **RTL** | Фаззинг (Verilator) | Неформальный |
| **RTL** | UVM | Неформальный |
| **RTL** | CDC/RDC | Формальный |
| **RTL** | Формальная проверка эквивалентности (Yosys) | Формальный |
| **RTL** | Формальная верификация свойств (FPV, Yosys) | Формальный |
| **RTL** | Полуформальная верификация (BMC) | Полуформальный |
| **Gate-level** | Логическая эквивалентность RTL и нетлиста (LEC) | Формальный |
| **Gate-level** | Временная симуляция на уровне вентилей | Неформальный |
| **Gate-level** | Статический временной анализ (STA) | Формальный |
| **Physical** | Физическая верификация / DRC/LVS | Формальный |
| **Post-silicon** | Функциональная валидация на стенде (ATE) | Неформальный |
| **Post-silicon** | Отладка по трассам (trace debugging) | Неформальный |

## Список источников для дальнейшего изучения

- Z3: https://github.com/Z3Prover/z3
- De Moura & Bjorner (2008). "Z3: An Efficient SMT Solver"
- Hennessy & Patterson, "Computer Architecture: A Quantitative Approach"
- gem5 simulator: https://www.gem5.org/
- Lamport, "Specifying Systems" (2002)
- TLA+ Home: https://lamport.azurewebsites.net/tla/tla.html
- Newcombe et al., "How Amazon Web Services Uses Formal Methods" (2015)
- Fagan, "Design and Code Inspections to Reduce Errors in Program Development" (1976)
- RISC-V ISA Specification Review Guidelines
- IEEE 1028-2008: Standard for Software Reviews and Audits
- IEEE 1666-2011: Standard SystemC Language Reference Manual
- Accellera SystemC: https://github.com/accellera-official/systemc
- RISC-V ISA Sim (Spike): https://github.com/riscv-software-src/riscv-isa-sim
- Sail: https://github.com/riscv/sail-riscv
- Gray et al., "Sail: a language for ISA specifications"
- RISC-V Formal: https://github.com/YosysHQ/riscv-formal
- Alglave et al., "Herding cats: modelling, simulation, testing, and data-mining for weak memory" (2014)
- RISC-V RVWMO Spec: https://github.com/riscv/riscv-isa-manual
- herdtools: https://github.com/herd7/herdtools7
- RISC-V Arch Test: https://github.com/riscv-non-isa/riscv-arch-test
- Spike (riscv-isa-sim): https://github.com/riscv-software-src/riscv-isa-sim
- HTIF: https://github.com/riscv-software-src/riscv-fesvr
- riscv-tests: https://github.com/riscv-software-src/riscv-tests
- Proxy Kernel: https://github.com/riscv-software-src/riscv-pk
- RISC-V DV (instruction stream generator): https://github.com/google/riscv-dv
- cocotb documentation: https://docs.cocotb.org/
- Bergeron, J. "Writing Testbenches: Functional Verification of HDL Models", 2nd ed., Springer 2006.
- IEEE 1800-2023, Chapter 19: "Functional Coverage"
- IEEE Std 1364-2005 – Verilog HDL
- Bergeron, J. – Writing Testbenches using SystemVerilog
- Sutherland, S. – Verilog and SystemVerilog Gotchas
- Icarus Verilog: https://github.com/steveicarus/iverilog
- Rocket Chip: https://github.com/chipsalliance/rocket-chip
- VexRiscv: https://github.com/SpinalHDL/VexRiscv
- NEORV32: https://github.com/stnolting/neorv32
- SERV (bit-serial RISC-V): https://github.com/olofk/serv
- Western Digital SweRV: https://github.com/westerndigitalcorporation/swerv_eh1
- Miller, B. et al. – "Fuzz Revisited: A Re-examination of the Reliability of UNIX Utilities and Services" (1995)
- Google RISC-V DV: https://github.com/google/riscv-dv
- IEEE Std 1800-2017 – SystemVerilog (глава "Randomization")
- IEEE 1800.2-2020: UVM Standard
- OpenHW Group CORE-V UVM: https://github.com/openhwgroup/core-v-verif
- Verification Academy: https://verificationacademy.com/
- Ginosar, R. "Fourteen Ways to Fool Your Synchronizer", ASYNC 2003.
- Synopsys SpyGlass CDC User Guide
- Cummings, C. "Clock Domain Crossing (CDC) Design & Verification Techniques", SNUG 2008.
- Yosys manual: "Miter-based equivalence checking"
- Kuehlmann, A. et al. "Robust Boolean reasoning for equivalence checking", DAC 2002.
- SymbiYosys: https://github.com/YosysHQ/symbiyosys
- Biere, A. et al. "Bounded Model Checking", Advances in Computers, 2003.
- Yosys Formal: https://yosyshq.readthedocs.io/projects/yosys/en/latest/formal.html
- RISC-V Formal: https://github.com/YosysHQ/riscv-formal
- Synopsys Formality User Guide
- Cadence Conformal LEC User Guide
- IEEE Std 1497-1999 (SDF Standard)
- Synopsys VCS SDF Annotation Guide
- Icarus Verilog documentation: http://iverilog.icarus.com/
- OpenSTA: https://github.com/The-OpenSTA-Project/OpenSTA
- OpenTimer: https://github.com/OpenTimer/OpenTimer
- Synopsys Design Constraints (SDC) – IEEE Std 1801-2015
- Liberty Library Format – IEEE Std 1603-2003
- SkyWater PDK (OpenSource): https://github.com/google/skywater-pdk
- Nangate 45nm Open Cell Library: https://si2.org/open-cell-library/
- Static Timing Analysis for Nanometer Designs – J. Bhasker, R. Chadha
- Calibre (Siemens EDA): https://eda.sw.siemens.com/calibre
- Cadence PVS: https://www.cadence.com/pvs
- Synopsys ICV: https://www.synopsys.com/icv
- KLayout: https://www.klayout.de
- Magic VLSI: http://opencircuitdesign.com/magic
- SkyWater PDK documentation: https://skywater-pdk.readthedocs.io
- IEEE 1500 – Standard for Embedded Core Test
- IEEE 1149.1 (JTAG) – Standard Test Access Port and Boundary-Scan Architecture
- Bushnell, M., Agrawal, V. – Essentials of Electronic Testing for Digital, Memory & Mixed-Signal VLSI Circuits
- Advantest: https://www.advantest.com
- Teradyne: https://www.teradyne.com
- RISC-V Trace Specification: https://github.com/riscv-non-isa/tg-nexus-trace
- ARM CoreSight: https://developer.arm.com/coresight
- Nexus 5001 Standard – IEEE-ISTO 5001
- Lauterbach TRACE32: https://www.lauterbach.com
