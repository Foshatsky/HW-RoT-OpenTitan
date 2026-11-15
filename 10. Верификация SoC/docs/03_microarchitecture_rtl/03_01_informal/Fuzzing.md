### МЕТОД: Фаззинг (fuzzing / генерация случайных инструкций)
### КАТЕГОРИЯ: 3. Микроархитектурная и RTL-верификация / Неформальная
### ФОРМАЛЬНОСТЬ: полуформальный

### СУТЬ МЕТОДА:
Фаззинг это автоматизированный метод тестирования, при котором на вход верифицируемого модуля подаётся большое количество псевдослучайных стимулов. Результаты каждого теста сравниваются с эталонной моделью, реализующей ту же спецификацию без намеренных дефектов. Метод эффективен для обнаружения редких граничных условий и нетривиальных логических ошибок, которые сложно найти ручным направленным тестированием.

В контексте верификации процессоров фаззинг обычно реализуется как генерация случайных последовательностей инструкций со случайными операндами. Каждая сгенерированная последовательность подается одновременно на тестируемое устройство (RTL или симулятор) и на эталонную модель (Spike, Sail, QEMU). Результаты выполнения (значения регистров, состояние памяти, флаги) сравниваются; любое расхождение указывает на ошибку в тестируемом устройстве или эталоне.

Главное преимущество фаззинга перед направленными тестами – способность находить ошибки в неожиданных комбинациях событий. Например, фаззер может сгенерировать последовательность, где переполнение кэша инструкций совпадает с прерыванием и неверно предсказанным переходом, что приводит к состоянию гонки, не описанному в спецификации. Направленный тест, написанный инженером, с очень малой вероятностью покроет такую комбинацию.

Одной из разновидностей фаззинга является фаззинг с управлением по покрытию, когда генератор использует обратную связь от покрытия кода (покрытие строк кода, покрытие переключений сигналов, покрытие состояний конечного автомата) для направления генерации в непокрытые области. Такой подход значительно повышает эффективность фаззинга и используется в современных инструментах вроде RISC-V DV (Google), Whisper и Torture.

Основные ограничения фаззинга: отсутствие гарантии полноты покрытия (метод вероятностный, а не формальный), необходимость эталонной модели, трудность генерации осмысленных последовательностей для сложных структур (MMU, TLB, виртуальная память) и высокие вычислительные затраты для достижения глубокого покрытия больших проектов.

### ИСТОРИЧЕСКАЯ СПРАВКА:
Понятие «fuzzing» введено Бартоном Миллером (Висконсинский университет) в 1988 г. для тестирования устойчивости UNIX-утилит к случайным входным данным. В верификации аппаратуры метод начал активно применяться с 2000-х годов, особенно после внедрения рандомизированных testbench-ов на SystemVerilog и UVM. Современные процессорные фаззеры (RISC-V DV, Google, Whisper) генерируют случайные последовательности инструкций и сравнивают поведение разных моделей (RTL против ISA-симулятора).

### ПОДРОБНОЕ ОПИСАНИЕ МЕТОДА И ЭКСПЕРИМЕНТА:
Рассмотрим такой подход к верификации на практическом примере. В качестве объекта тестирования выбран 4-битный АЛУ (tiny_alu), выполняющий операции ADD, SUB, AND, OR, XOR над 4-битными операндами. Модуль намеренно содержит дефект: при SUB, если результат равен 0 (т.е. a == b), флаг carry_flag принудительно устанавливается в 1, хотя правильное значение borrow = 0.

Тестовое окружение:
* Использует [Verilator](../../tools_instructions/verilator.md) для компиляции Verilog-модуля в C++.
* Генерирует 500 случайных комбинаций (a, b, opcode) через rand() с seed = time().
* Подаёт каждую комбинацию на DUT и на эталонную реализацию на C++.
* Сравнивает result и carry_flag; при расхождении сообщает о mismatch.
* Выводит статистику: количество пройденных/проваленных тестов.

Код АЛУ на Verilog:

```systemverilog
module tiny_alu (
   input  wire [3:0] a,
   input  wire [3:0] b,
   input  wire [2:0] op,
   output reg  [3:0] result,
   output reg        carry_flag
);
   always @(*) begin
       carry_flag = 1'b0;
       result     = 4'b0;

       case (op)
           3'b000: {carry_flag, result} = a + b;

           3'b001: begin // SUB – intentionally buggy
               result = a - b;
               carry_flag = (a < b);
               // BUG: check result==0 *before* borrow is resolved;
               // when a==b we override the correct borrow=0 with 1
               if (result == 0)
                   carry_flag = 1;
           end

           3'b010: result = a & b;
           3'b011: result = a | b;
           3'b100: result = a ^ b;
           default: result = 4'b0;
       endcase
   end
endmodule
```

Тестовое окружение:

```cpp
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstdint>

// Verilator headers
#include <verilated.h>
#include "Vtiny_alu.h"

// ---------------------------------------------------------------
// Reference (golden) model – the same ALU spec, but correct
// ---------------------------------------------------------------
static void ref_alu(uint8_t a, uint8_t b, uint8_t op,
                   uint8_t &result, bool &carry) {
   switch (op) {
   case 0: // ADD
       result = (a + b) & 0xF;
       carry  = (static_cast<uint16_t>(a) + b) > 15;
       break;
   case 1: // SUB – correct: borrow = (a < b)
       result = (a - b) & 0xF;
       carry  = (a < b);
       break;
   case 2: // AND
       result = a & b;
       carry  = false;
       break;
   case 3: // OR
       result = a | b;
       carry  = false;
       break;
   case 4: // XOR
       result = a ^ b;
       carry  = false;
       break;
   default:
       result = 0;
       carry  = false;
       break;
   }
}

// ---------------------------------------------------------------
int main(int argc, char *argv[]) {
   srand(static_cast<unsigned>(time(nullptr)));

   Verilated::commandArgs(argc, argv);
   Vtiny_alu *dut = new Vtiny_alu;

   const int N = 500;
   int mismatches = 0;

   printf("Fuzzing tiny_alu – %d random test cases\n\n", N);

   for (int i = 0; i < N; i++) {
       // Random 4-bit operands and 3-bit opcode
       uint8_t a  = rand() & 0xF;
       uint8_t b  = rand() & 0xF;
       uint8_t op = rand() & 0x7;

       // Drive DUT
       dut->a = a;
       dut->b = b;
       dut->op = op;
       dut->eval();

       uint8_t dut_result = static_cast<uint8_t>(dut->result);
       bool    dut_carry  = dut->carry_flag != 0;

       // Reference
       uint8_t ref_result;
       bool    ref_carry;
       ref_alu(a, b, op, ref_result, ref_carry);

       // Compare
       if (dut_result != ref_result || dut_carry != ref_carry) {
           printf("MISMATCH #%d  a=%02x b=%02x op=%d  "
                  "DUT: result=%02x carry=%d  REF: result=%02x carry=%d\n",
                  ++mismatches, a, b, op,
                  dut_result, dut_carry,
                  ref_result, ref_carry);
       }

       // Progress every 100
       if ((i + 1) % 100 == 0)
           printf("  %d / %d  (%d mismatches so far)\n",
                  i + 1, N, mismatches);
   }

   printf("\n--- Fuzzing complete ---\n");
   if (mismatches == 0)
       printf("All %d tests PASSED.\n", N);
   else
       printf("%d MISMATCH(ES) found out of %d tests.\n", mismatches, N);

   delete dut;
   return mismatches ? 1 : 0;
}
```

### ЗАПУСК И ВЫВОД:
```
Fuzzing tiny_alu – 500 random test cases

MISMATCH #1  a=06 b=06 op=1  DUT: result=00 carry=1  REF: result=00 carry=0
MISMATCH #2  a=0b b=0b op=1  DUT: result=00 carry=1  REF: result=00 carry=0
 100 / 500  (2 mismatches so far)
 200 / 500  (2 mismatches so far)
MISMATCH #3  a=00 b=00 op=1  DUT: result=00 carry=1  REF: result=00 carry=0
 300 / 500  (3 mismatches so far)
MISMATCH #4  a=0d b=0d op=1  DUT: result=00 carry=1  REF: result=00 carry=0
 400 / 500  (4 mismatches so far)
 500 / 500  (4 mismatches so far)

--- Fuzzing complete ---
4 MISMATCH(ES) found out of 500 tests.
```

### АНАЛИЗ РЕЗУЛЬТАТОВ:
Фаззер выявил 4 несовпадения из 500 случайных тестов, и все они относятся к одному классу дефекта: при a == b (результат SUB равен 0) carry_flag ошибочно устанавливается в 1 вместо 0. Это соответствует преднамеренно внесенной ошибке, при которой проверка result == 0 выполнялась до корректного вычисления borrow. Прочие операции (ADD, AND, OR, XOR) и случаи SUB с a != b отработали без ошибок.

### ВЫВОД ПО МЕТОДУ:
Итак, фаззинг это эффективный метод поиска редких граничных условий. В данном эксперименте он потребовал всего 500 случайных тестов, чтобы многократно воспроизвести дефект. Однако для покрытия всех угловых случаев в сложных процессорных ядрах требуется на несколько порядков больше случайных стимулов + directed-тесты для труднодостижимых состояний.

### ДОПОЛНИТЕЛЬНЫЕ ИСТОЧНИКИ:
1. Miller, B. et al. — "Fuzz Revisited: A Re-examination of the Reliability of UNIX Utilities and Services" (1995)
2. Google RISC-V DV: https://github.com/google/riscv-dv
3. IEEE Std 1800-2017 — SystemVerilog (глава "Randomization")
