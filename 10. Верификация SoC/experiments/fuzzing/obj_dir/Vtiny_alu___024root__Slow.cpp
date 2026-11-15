// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vtiny_alu.h for the primary calling header

#include "Vtiny_alu___024root.h"
#include "Vtiny_alu__Syms.h"

//==========


void Vtiny_alu___024root___ctor_var_reset(Vtiny_alu___024root* vlSelf);

Vtiny_alu___024root::Vtiny_alu___024root(const char* _vcname__)
    : VerilatedModule(_vcname__)
 {
    // Reset structure values
    Vtiny_alu___024root___ctor_var_reset(this);
}

void Vtiny_alu___024root::__Vconfigure(Vtiny_alu__Syms* _vlSymsp, bool first) {
    if (false && first) {}  // Prevent unused
    this->vlSymsp = _vlSymsp;
}

Vtiny_alu___024root::~Vtiny_alu___024root() {
}

void Vtiny_alu___024root___eval_initial(Vtiny_alu___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtiny_alu__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtiny_alu___024root___eval_initial\n"); );
}

void Vtiny_alu___024root___combo__TOP__1(Vtiny_alu___024root* vlSelf);

void Vtiny_alu___024root___eval_settle(Vtiny_alu___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtiny_alu__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtiny_alu___024root___eval_settle\n"); );
    // Body
    Vtiny_alu___024root___combo__TOP__1(vlSelf);
}

void Vtiny_alu___024root___final(Vtiny_alu___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtiny_alu__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtiny_alu___024root___final\n"); );
}

void Vtiny_alu___024root___ctor_var_reset(Vtiny_alu___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtiny_alu__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtiny_alu___024root___ctor_var_reset\n"); );
    // Body
    vlSelf->a = VL_RAND_RESET_I(4);
    vlSelf->b = VL_RAND_RESET_I(4);
    vlSelf->op = VL_RAND_RESET_I(3);
    vlSelf->result = VL_RAND_RESET_I(4);
    vlSelf->carry_flag = VL_RAND_RESET_I(1);
}
