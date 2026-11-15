// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Symbol table internal header
//
// Internal details; most calling programs do not need this header,
// unless using verilator public meta comments.

#ifndef VERILATED_VTINY_ALU__SYMS_H_
#define VERILATED_VTINY_ALU__SYMS_H_  // guard

#include "verilated_heavy.h"

// INCLUDE MODEL CLASS

#include "Vtiny_alu.h"

// INCLUDE MODULE CLASSES
#include "Vtiny_alu___024root.h"

// SYMS CLASS (contains all model state)
class Vtiny_alu__Syms final : public VerilatedSyms {
  public:
    // INTERNAL STATE
    Vtiny_alu* const __Vm_modelp;
    bool __Vm_didInit = false;

    // MODULE INSTANCE STATE
    Vtiny_alu___024root            TOP;

    // CONSTRUCTORS
    Vtiny_alu__Syms(VerilatedContext* contextp, const char* namep, Vtiny_alu* modelp);
    ~Vtiny_alu__Syms();

    // METHODS
    const char* name() { return TOP.name(); }
} VL_ATTR_ALIGNED(VL_CACHE_LINE_BYTES);

#endif  // guard
