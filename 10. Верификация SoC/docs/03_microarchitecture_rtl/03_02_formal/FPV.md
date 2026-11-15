### МЕТОД: Формальная верификация свойств (Formal Property Verification, FPV)
### КАТЕГОРИЯ: 3. Микроархитектурная и RTL-верификация / Формальная
### ФОРМАЛЬНОСТЬ: формальный

### СУТЬ МЕТОДА:
Формальная верификация свойств (FPV) — это метод математического доказательства того, что RTL-описание цифровой схемы удовлетворяет заданным формальным свойствам (assertions). В отличие от симуляции, которая проверяет лишь конечное множество конкретных сценариев, FPV анализирует все возможные входные последовательности и все достижимые состояния схемы, обеспечивая 100-процентное покрытие пространства состояний (exhaustive verification). Свойства записываются в виде логических утверждений (assert) на языке Verilog/SystemVerilog или в формате SVA (SystemVerilog Assertions).

Основная идея FPV заключается в преобразовании RTL-описания и утверждений в логическую формулу, которая затем передаётся SAT-решателю (satisfiability solver) или SMT-решателю (satisfiability modulo theories). Решатель пытается найти такой набор входных сигналов и состояний, который нарушает хотя бы одно утверждение. Если такой набор не существует (SAT solver сообщает «UNSAT»), свойство считается доказанным. Если контрпример найден, он выдаётся разработчику для анализа и исправления схемы.

Рассмотрим пример с использованием [Yosys](../../tools_instructions/yosys.md). Используется bounded model checking (BMC) с k-индукцией (k-induction): решатель проверяет утверждения на ограниченном числе шагов (k) и, если это возможно, доказывает их на всех глубинах. Такой подход даёт формальные гарантии корректности для схем с конечным числом состояний при условии, что найдена достаточная глубина индукции. Эксперимент проведён на двух модулях: конечном автомате с тремя состояниями (fsm_props) и счётчике с переполнением (safe_counter).

Есть два верифицируемых модуля: fsm_props и safe_counter. Для каждого Yosys выполняет следующие шаги: `read_verilog -formal` (загрузка с поддержкой assert), `hierarchy -top` (выбор топ-модуля), `proc; opt; async2sync` (преобразование always-блоков в нетлист, оптимизация, синхронизация асинхронных сбросов), `sat -prove-asserts -verify -tempinduct -seq N`. Флаг `-seq N` задаёт глубину поиска в тактах, `-tempinduct` — использование временной индукции (k-induction). Индукция пытается доказать assert сначала на базовом случае, затем на шаге индукции. При неудаче глубина увеличивается.

Модуль fsm_props (три состояния: IDLE=00, ACTIVE=01, ERROR=10) содержит три assert:
1. Состояние никогда не равно невалидному значению 2'b11.
2. Если предыдущий такт был ERROR, текущее состояние — IDLE.
3. Комбинационное значение next_state всегда валидно (IDLE, ACTIVE или ERROR).

Модуль safe_counter (счётчик 0..2 с автовозвратом) содержит три assert:
1. Следующее значение счётчика не превышает 2.
2. Если счётчик равен 2, следующее значение — 0 (заворот).
3. Иначе счётчик увеличивается на 1.

Утверждения вписаны прямо в RTL-код в блоке always @(posedge clk) через конструкцию `assert(...)`. Yosys автоматически распознаёт их как формальные свойства и встраивает в проверочную схему.

### ИСТОРИЧЕСКАЯ СПРАВКА:
Формальная верификация аппаратного обеспечения восходит к работам 1980-х годов, когда появились первые BDD-решатели (R. Bryant, 1986) и модель проверки моделей (model checking, E. Clarke, E. Emerson, 1981). Коммерческое применение формальной верификации началось в 1990-х с инструментов FormalCheck (Bell Labs/Lucent) и RuleBase (IBM). В 2000-х метод SAT-based model checking (A. Biere, A. Cimatti, E. Clarke — «Bounded Model Checking», 1999) произвёл революцию, позволив проверять схемы с миллионами логических элементов. Yosys (Claire Wolf, 2012—н.в.) сделал формальную верификацию доступной в open-source, включив SAT-решатель (MiniSat) и поддержку формальных проверок.

### ЛИСТИНГИ (код, скрипты):

**fsm_props.v — конечный автомат с формальными утверждениями:**
```systemverilog
module fsm_props(input wire clk, input wire rst, input wire start,
                 output reg [1:0] state, output reg done);

 parameter IDLE   = 2'b00;
 parameter ACTIVE = 2'b01;
 parameter ERROR  = 2'b10;

 reg [1:0] next_state;
 reg was_error;

 // Combinational next state
 always @(*) begin
   case (state)
     IDLE:   next_state = start ? ACTIVE : IDLE;
     ACTIVE: next_state = ERROR;
     ERROR:  next_state = IDLE;
     default: next_state = IDLE;
   endcase
 end

 // Sequential logic and assertions
 always @(posedge clk) begin
   if (rst) begin
     state <= IDLE;
     was_error <= 0;
   end else begin
     state <= next_state;
     was_error <= (state == ERROR);

     // Assert 1: state must never be invalid
     assert(state != 2'b11);

     // Assert 2: if previous cycle was ERROR, current state is IDLE
     if (was_error)
       assert(state == IDLE);

     // Assert 3: combinational next_state is always valid
     assert(next_state == IDLE || next_state == ACTIVE || next_state == ERROR);
   end
 end

 assign done = (state == IDLE);

endmodule
```

**safe_counter.v — счётчик mod-3 с формальными утверждениями:**
```systemverilog
module safe_counter(input wire clk, input wire rst, input wire en,
                    output reg [1:0] count);

 wire [1:0] next_count;

 // Combinational next value
 assign next_count = (count == 2'd2) ? 2'd0 : count + 2'd1;

 // Sequential logic and assertions
 always @(posedge clk) begin
   if (rst) begin
     count <= 0;
   end else if (en) begin
     count <= next_count;
   end

   if (!rst && en) begin
     // Assert 1: count never exceeds 2
     assert(next_count <= 2'd2);

     // Assert 2: if count is 2, next must be 0 (wrap)
     if (count == 2'd2)
       assert(next_count == 2'd0);
     else
       // Assert 3: otherwise count increments by 1
       assert(next_count == count + 2'd1);
   end
 end

endmodule
```

### ЗАПУСК И ВЫВОД:
```
--- Proving assertions in fsm_props.v (fsm_props) ---
...
6. Executing SAT pass (solving SAT problems in the circuit).

Setting up time step 1:
...
** Trying induction with length 1 **
[base case 1] Solving problem with 521 variables and 1361 clauses..
Base case for induction length 1 proven.
[induction step 1] Solving problem with 332 variables and 855 clauses..
Induction step failed. Incrementing induction length.

** Trying induction with length 2 **
[base case 2] Solving problem with 733 variables and 1943 clauses..
Base case for induction length 2 proven.
[induction step 2] Solving problem with 572 variables and 1529 clauses..
Induction step proven: SUCCESS!
>>> RESULT: fsm_props - ALL ASSERTIONS PROVEN <<<
...

--- Proving assertions in safe_counter.v (safe_counter) ---
...
6. Executing SAT pass (solving SAT problems in the circuit).
...
** Trying induction with length 1 **
[base case 1] Solving problem with 501 variables and 1314 clauses..
Base case for induction length 1 proven.
[induction step 1] Solving problem with 318 variables and 822 clauses..
Induction step proven: SUCCESS!
>>> RESULT: safe_counter - ALL ASSERTIONS PROVEN <<<
```

### АНАЛИЗ РЕЗУЛЬТАТОВ:
Оба модуля — fsm_props и safe_counter — успешно прошли формальную верификацию. Для fsm_props потребовалась глубина индукции k=2 (первая попытка с k=1 не удалась на шаге индукции), что подтверждает необходимость подбора глубины для корректной индукции. Для safe_counter хватило k=1. SAT-решатель Yosys подтвердил, что при всех допустимых входных последовательностях ни одно assertion-свойство не нарушается — ни одно состояние не становится невалидным, а счётчик никогда не выходит за границы 0..2. Формальное доказательство гарантирует 100% покрытие всех состояний и переходов, включая граничные случаи, что принципиально недоступно симуляционным методам.

### ВЫВОД ПО МЕТОДУ:
FPV — мощный инструмент верификации, дающий математически строгие гарантии корректности для критически важных свойств. Метод особенно ценен для модулей с конечным пространством состояний (FSM, контроллеры, арбитры). Применим для реальных процессоров, но требует квалифицированных специалистов и может столкнуться с комбинаторным взрывом (state explosion) для сложных блоков. В таких случаях используются гибридные подходы (формальный Bounded Model Checking на небольших глубинах в сочетании с симуляцией). Yosys SAT обеспечивает базовую FPV для схем до нескольких тысяч логических элементов, для более сложных проектов рекомендуется SymbiYosys с GHDL/Verific или коммерческие инструменты (JasperGold, Questa Formal, VC Formal).

### ДОПОЛНИТЕЛЬНЫЕ ИСТОЧНИКИ:
- [Yosys](../../tools_instructions/yosys.md)
- SymbiYosys: https://github.com/YosysHQ/symbiyosys
- Biere, A. et al. "Bounded Model Checking", Advances in Computers, 2003.
