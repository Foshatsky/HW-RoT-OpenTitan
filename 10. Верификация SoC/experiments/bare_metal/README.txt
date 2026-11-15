=== EXPERIMENT: Bare-Metal Testing ===
Category: 3.03 Bare-metal testing (informal RTL verification)
Date: 2026-05-29

This experiment was covered by the Spike test in experiment 2.03 (Spike ISA simulator).

A bare-metal RISC-V program (sum_1_to_10.c) was written, compiled with
riscv64-linux-gnu-gcc, and executed on the Spike simulator (via pk).

The program computes the sum of integers from 1 to 10 and returns the
result in register a0 (expected: 55). Spike confirmed correct execution.

For RTL verification the same binary should be run on an RTL simulator
(Verilator, VCS, etc.) and the final register/memory state compared
against the Spike golden reference.

See:
  - experiments/isa/spike_spike_test/ (experiment 2.03)
  - docs/02_isa/02_03_spike.txt
  - docs/03_microarchitecture_rtl/03_01_informal/Bare_metal_testing.txt
