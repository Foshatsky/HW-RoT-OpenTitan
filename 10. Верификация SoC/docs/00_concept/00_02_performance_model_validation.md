### МЕТОД: Валидация модели производительности
### КАТЕГОРИЯ: 0. Концепция
### ФОРМАЛЬНОСТЬ: неформальный

### СУТЬ МЕТОДА:

Этот метод позволяет оценить производительность проектируемого процессора еще до того, как будет написан RTL-код и проведена детальная симуляция. На этапе архитектурного исследования (а часто еще на этапе разработки концепции) крайне полезно будет понять, достигнет ли целевая микроархитектура целевых показателей по количеству тактов на инструкцию (CPI – Cycles Per Instruction), пропускной способности и задержкам. Без модели производительности разработчик рискует выбрать неудачную микроархитектуру и обнаружить проблему лишь на поздних этапах, когда исправление требует огромных затрат.

На высоком уровне метод заключается в построении упрощенной математической или симуляционной модели конвейера процессора, которая учитывает основные эффекты, влияющие на производительность: конфликты по данным, конфликты по управлению (штрафы за неверное предсказание ветвлений – кстати, по этой метрике архитектура RISC за счет небольшого количества инструкций, как правило, выигрывает у CISC), конфликты по структурным ресурсам. Модель запускается на тестовых программах (микробенчмарках) и предсказывает общее число тактов или CPI. Эти предсказания затем сравниваются с результатами потактово-точной симуляции или, на поздних этапах, с замерами на реальном кремнии.

Входными данными является описание микроархитектуры (количество стадий конвейера, задержки всех блоков, механизмы предсказания переходов), тестовая программа (последовательность инструкций) и начальное состояние памяти и регистров. Выходными данными – общее количество тактов, CPI, количество конфликтов каждого типа, загрузка функциональных блоков. Модель может выдавать как абсолютные значения, так и относительные сравнения различных архитектурных вариантов.

Важно отметить, что программы (микробенчмарки) в действительности не выполняются, вместо этого подсчитывается, сколько примерно тактов уйдет на их выполнение.

Метод используется на ранних этапах проектирования (архитектурное исследование и анализ компромиссов) и позволяет за часы оценить эффект от изменений, которые потребовали бы длительной RTL-симуляции. Сильные стороны: высокая скорость анализа, возможность сравнения десятков архитектурных альтернатив, низкие требования к вычислительным ресурсам, отсутствие необходимости в написании RTL-кода для каждого варианта. Слабые стороны: точность зависит от полноты учтенных эффектов – упрощённые модели не учитывают кэши, внеочередное исполнение, спекулятивные вычисления, что требует калибровки по более точным симуляторам (gem5, McPAT) на поздних этапах.

### ИСТОРИЧЕСКАЯ СПРАВКА:

Первые модели производительности появились в 1960-х (IBM Stretch, CDC 6600). Ключевые работы: Hennessy & Patterson "Computer Architecture: A Quantitative Approach" (1990) – заложили основы CPI-анализа. Современные инструменты: gem5 (моделирование full-system), GWT (C*), архитектурные эксплореры. В индустрии используется для trade-off анализа перед RTL-реализацией.

### ПОДРОБНОЕ ОПИСАНИЕ МЕТОДА И ЭКСПЕРИМЕНТА:

Инструмент: Python 3.12 (без внешних зависимостей).

Построена cycle-approximate модель 5-стадийного конвейера RISC-V с базовым CPI = 1, штрафом 1 цикл за RAW-hazard и штрафом 2 цикла за неверное предсказание перехода.

Тестовая программа – вычисление суммы 1..10 (33 инструкции с циклом). Симуляция дала 64 цикла. Аналитическая формула CPI = 1 + (stalls + br*2)/IC предсказала 65 циклов – ошибка 1.6%. При подстановке реальных значений stalls и branches из симуляции модель выдает ровно 64 цикла (ошибка 0%).

Python-код:

### ЛИСТИНГИ (код, скрипты):

```python
def simulate(program):
   """
   Cycle-approximate simulation.
   Returns: (total_cycles, instr_count, stall_count, branch_count)
   """
   regs = [0] * 32
   pc = 0
   cycles = 0
   stalls = 0
   branches = 0
   instr_count = 0
   mem = {}


   def r(x):
       return regs[x] if x != 0 else 0


   def w(x, v):
       if x != 0:
           regs[x] = v


   # track last writer for hazard detection
   last_wr = None


   while pc < len(program):
       instr = program[pc]
       op, rd, rs1, rs2, imm = instr


       # Hazard: if last instruction wrote a register this one reads
       hazard = False
       if last_wr is not None and last_wr != 0:
           if rs1 == last_wr or rs2 == last_wr:
               hazard = True


       if hazard:
           stalls += 1
           cycles += 1
           last_wr = None  # bubble resolves hazard
           continue


       # Execute instruction
       instr_count += 1
       if op == 'add':
           w(rd, r(rs1) + r(rs2))
       elif op == 'sub':
           w(rd, r(rs1) - r(rs2))
       elif op == 'addi':
           w(rd, r(rs1) + imm)
       elif op == 'lw':
           addr = r(rs1) + imm
           w(rd, mem.get(addr, 0))
       elif op == 'sw':
           addr = r(rs1) + imm
           mem[addr] = r(rs2)
       elif op == 'beq':
           branches += 1
           if r(rs1) == r(rs2):
               pc += imm - 1  # -1 because pc++ below
           cycles += 2  # branch penalty
       elif op == 'bne':
           branches += 1
           if r(rs1) != r(rs2):
               pc += imm - 1
           cycles += 2
       elif op == 'nop':
           pass


       last_wr = rd if op not in ('sw', 'beq', 'bne', 'nop') else None
       pc += 1
       cycles += 1


   return cycles, instr_count, stalls, branches, regs




def analytical(instr_count, stalls, branches, bp=2):
   ideal = instr_count
   penalties = stalls + branches * bp
   total = ideal + penalties
   cpi = total / instr_count if instr_count else 0
   return cpi, total




def main():
   print("=" * 60)
   print("PERFORMANCE MODEL VALIDATION")
   print("=" * 60)


   # Program: sum 1..10
   # x1=sum, x2=counter
   # Loop: add x1,x1,x2; addi x2,x2,-1; bnez x2, loop
   prog = [
       ('addi', 2, 0, 0, 10),      # x2 = 10
       ('addi', 1, 0, 0, 0),       # x1 = 0
       ('add',  1, 1, 2, 0),       # loop: x1 += x2
       ('addi', 2, 2, 0, -1),      # x2 -= 1
       ('bne',  0, 2, 0, -2),      # if x2 != 0, goto loop
       ('nop',  0, 0, 0, 0),       # fin
   ]


   cycles, icount, stalls, branches, regs = simulate(prog)
   actual_cpi = cycles / icount


   # Analytical prediction
   pred_stalls = 10
   pred_branches = 11
   pred_cpi, pred_cycles = analytical(icount, pred_stalls, pred_branches)


   print(f"\nProgram: sum 1..10 (expected x1=55, x2=0)")
   print(f"  Instructions fetched: {len(prog)}")
   print(f"  Instructions executed: {icount}")
   print(f"  Simulation cycles:     {cycles}")
   print(f"  Actual CPI:            {actual_cpi:.3f}")
   print(f"  RAW stalls:            {stalls}")
   print(f"  Branch count:          {branches}")
   print(f"  Final x1 = {regs[1]} {'PASS' if regs[1]==55 else 'FAIL'}")
   print(f"  Final x2 = {regs[2]} {'PASS' if regs[2]==0 else 'FAIL'}")


   print(f"\n--- Analytical CPI model ---")
   print(f"  Formula: CPI = 1 + (stalls + branches * 2) / instr_count")
   print(f"  Predicted stalls:  {pred_stalls}")
   print(f"  Predicted branches: {pred_branches}")
   print(f"  Predicted cycles:   {pred_cycles}")
   print(f"  Predicted CPI:      {pred_cpi:.3f}")
   print(f"  Prediction error:   {abs(cycles - pred_cycles)} cycles "
         f"({abs(cycles - pred_cycles) / cycles * 100:.1f}%)")


   print(f"\n--- Model refinement ---")
   actual_stalls = stalls
   actual_branches = branches
   refined_cpi, refined_cycles = analytical(icount, actual_stalls, actual_branches)
   print(f"  Using actual stalls ({actual_stalls}) and branches ({actual_branches}):")
   print(f"  Refined cycles: {refined_cycles}")
   print(f"  Refined CPI: {refined_cpi:.3f}")
   print(f"  Error: {abs(cycles - refined_cycles)} cycles "
         f"({abs(cycles - refined_cycles) / cycles * 100:.1f}%)")




if __name__ == "__main__":
   main()
```

### ЗАПУСК И ВЫВОД:

Результат выполнения:

```
============================================================
PERFORMANCE MODEL VALIDATION
============================================================


Program: sum 1..10 (expected x1=55, x2=0)
  Instructions fetched: 6
  Instructions executed: 33
  Simulation cycles:     64
  Actual CPI:            1.939
  RAW stalls:            11
  Branch count:          10
  Final x1 = 55 PASS
  Final x2 = 0 PASS


--- Analytical CPI model ---
  Formula: CPI = 1 + (stalls + branches * 2) / instr_count
  Predicted stalls:  10
  Predicted branches: 11
  Predicted cycles:   65
  Predicted CPI:      1.970
  Prediction error:   1 cycles (1.6%)


--- Model refinement ---
  Using actual stalls (11) and branches (10):
  Refined cycles: 64
  Refined CPI: 1.939
  Error: 0 cycles (0.0%)
```

### АНАЛИЗ РЕЗУЛЬТАТОВ:

Эксперимент подтверждает, что даже простая циклическая модель позволяет быстро (за секунды) оценивать производительность с точностью ~2%, а после калибровки – абсолютно точно (в нашем случае, на практике такое встречается нечасто). Метод полезен для раннего архитектурного анализа, хоть и не учитывает кэши, внеочередное исполнение и спекуляцию.

### ВЫВОД ПО МЕТОДУ:

Метод важен для архитектурного исследования: позволяет за часы (а не недели RTL-симуляции) оценить эффект от изменений в микроархитектуре. Ограничения: точность зависит от полноты учтённых эффектов (кэши, внеочередное исполнение, спекуляция). Для реальных процессоров используются gem5 или McPAT.

### ДОПОЛНИТЕЛЬНЫЕ ИСТОЧНИКИ:

- Hennessy & Patterson, "Computer Architecture: A Quantitative Approach"
- gem5 simulator: https://www.gem5.org/
