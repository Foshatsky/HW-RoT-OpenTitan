#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstdint>

// Verilator headers
#include <verilated.h>
#include "Vtiny_alu.h"

// ---------------------------------------------------------------
// Reference (golden) model — the same ALU spec, but correct
// ---------------------------------------------------------------
static void ref_alu(uint8_t a, uint8_t b, uint8_t op,
                    uint8_t &result, bool &carry) {
    switch (op) {
    case 0: // ADD
        result = (a + b) & 0xF;
        carry  = (static_cast<uint16_t>(a) + b) > 15;
        break;
    case 1: // SUB — correct: borrow = (a < b)
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

    printf("Fuzzing tiny_alu — %d random test cases\n\n", N);

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
