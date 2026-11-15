// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vtiny_alu.h for the primary calling header

#ifndef VERILATED_VTINY_ALU___024ROOT_H_
#define VERILATED_VTINY_ALU___024ROOT_H_  // guard

#include "verilated_heavy.h"

//==========

class Vtiny_alu__Syms;

//----------

VL_MODULE(Vtiny_alu___024root) {
  public:

    // PORTS
    VL_IN8(a,3,0);
    VL_IN8(b,3,0);
    VL_IN8(op,2,0);
    VL_OUT8(result,3,0);
    VL_OUT8(carry_flag,0,0);

    // INTERNAL VARIABLES
    Vtiny_alu__Syms* vlSymsp;  // Symbol table

    // CONSTRUCTORS
  private:
    VL_UNCOPYABLE(Vtiny_alu___024root);  ///< Copying not allowed
  public:
    Vtiny_alu___024root(const char* name);
    ~Vtiny_alu___024root();

    // INTERNAL METHODS
    void __Vconfigure(Vtiny_alu__Syms* symsp, bool first);
} VL_ATTR_ALIGNED(VL_CACHE_LINE_BYTES);

//----------


#endif  // guard
