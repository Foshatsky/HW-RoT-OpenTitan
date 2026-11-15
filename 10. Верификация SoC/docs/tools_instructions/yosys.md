# Yosys — Open SYnthesis Suite

Сборка из исходников (требует Ubuntu/Debian):

```bash
# Зависимости сборки
sudo apt install build-essential clang lld bison flex \
                 libreadline-dev gawk tcl-dev libffi-dev \
                 git pkg-config python3

git clone https://github.com/YosysHQ/yosys.git
cd yosys
make -j$(nproc)
make install PREFIX=$(pwd)/install
```

После сборки бинарник в `install/bin/yosys`. Используйте полный путь или добавьте в `PATH`.

Удаление: `rm -rf yosys/`

Используется в:
- [Equivalence_checking](../03_microarchitecture_rtl/03_02_formal/Equivalence_checking.md)
- [FPV](../03_microarchitecture_rtl/03_02_formal/FPV.md)
- [Semi_formal_BMC](../03_microarchitecture_rtl/03_03_semi-formal/Semi_formal_BMC.md)
- [04_01_logic_equivalence](../04_gate_level/04_01_logic_equivalence.md)
