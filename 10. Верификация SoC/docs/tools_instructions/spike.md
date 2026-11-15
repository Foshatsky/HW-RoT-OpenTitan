# Spike — RISC-V ISA Simulator

Сборка из исходников:

```bash
# Зависимости
sudo apt install build-essential device-tree-compiler

git clone https://github.com/riscv-software-src/riscv-isa-sim.git
cd riscv-isa-sim
mkdir build && cd build
../configure --prefix=$(pwd)/../install
make -j$(nproc)
make install
```

Бинарник: `riscv-isa-sim/install/bin/spike`.

Используется в:
- [02_03_architectural_testing_simulation](../02_architecture/02_03_architectural_testing_simulation.md)
- [Bare_metal_testing](../03_microarchitecture_rtl/03_01_informal/Bare_metal_testing.md)
