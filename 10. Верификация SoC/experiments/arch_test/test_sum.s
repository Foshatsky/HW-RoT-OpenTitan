.option nopic
.section .data
.align 3
.globl tohost
tohost:
    .dword 0
.globl fromhost
fromhost:
    .dword 0

.section .text
.globl _start
_start:
    li t0, 1
    li t1, 0
loop:
    add t1, t1, t0
    addi t0, t0, 1
    li t2, 11
    blt t0, t2, loop
    mv a0, t1
    la t2, tohost
    slli t1, t1, 1
    addi t1, t1, 1
    sd t1, 0(t2)
ebreak
