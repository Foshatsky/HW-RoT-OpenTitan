# Verilator — быстрый Verilog-симулятор

Версия из apt обычно старая. Рекомендуется сборка 4.210+ из исходников:

```bash
# Вариант 1: системный пакет (может быть старым)
sudo apt install verilator

# Вариант 2: сборка из исходников
sudo apt install build-essential git perl python3 \
                 flex bison libfl-dev help2man
git clone https://github.com/verilator/verilator.git
cd verilator
git checkout v4.210
autoconf && ./configure --prefix=$(pwd)/install
make -j$(nproc)
make install
```

Бинарник: `verilator/install/bin/verilator`.

Используется в:
- [Fuzzing](../03_microarchitecture_rtl/03_01_informal/Fuzzing.md)
