// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vtiny_alu.h for the primary calling header

#include "Vtiny_alu___024root.h"
#include "Vtiny_alu__Syms.h"

//==========

VL_INLINE_OPT void Vtiny_alu___024root___combo__TOP__1(Vtiny_alu___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtiny_alu__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtiny_alu___024root___combo__TOP__1\n"); );
    // Body
    vlSelf->carry_flag = 0U;
    vlSelf->result = 0U;
    if ((4U & (IData)(vlSelf->op))) {
        vlSelf->result = ((2U & (IData)(vlSelf->op))
                           ? 0U : ((1U & (IData)(vlSelf->op))
                                    ? 0U : ((IData)(vlSelf->a) 
                                            ^ (IData)(vlSelf->b))));
    } else if ((2U & (IData)(vlSelf->op))) {
        vlSelf->result = ((1U & (IData)(vlSelf->op))
                           ? ((IData)(vlSelf->a) | (IData)(vlSelf->b))
                           : ((IData)(vlSelf->a) & (IData)(vlSelf->b)));
    } else if ((1U & (IData)(vlSelf->op))) {
        vlSelf->result = (0xfU & ((IData)(vlSelf->a) 
                                  - (IData)(vlSelf->b)));
        vlSelf->carry_flag = ((IData)(vlSelf->a) < (IData)(vlSelf->b));
        if ((0U == (IData)(vlSelf->result))) {
            vlSelf->carry_flag = 1U;
        }
    } else {
        vlSelf->carry_flag = (1U & (((IData)(vlSelf->a) 
                                     + (IData)(vlSelf->b)) 
                                    >> 4U));
        vlSelf->result = (0xfU & ((IData)(vlSelf->a) 
                                  + (IData)(vlSelf->b)));
    }
}

void Vtiny_alu___024root___eval(Vtiny_alu___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtiny_alu__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtiny_alu___024root___eval\n"); );
    // Body
    Vtiny_alu___024root___combo__TOP__1(vlSelf);
}

QData Vtiny_alu___024root___change_request_1(Vtiny_alu___024root* vlSelf);

VL_INLINE_OPT QData Vtiny_alu___024root___change_request(Vtiny_alu___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtiny_alu__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtiny_alu___024root___change_request\n"); );
    // Body
    return (Vtiny_alu___024root___change_request_1(vlSelf));
}

VL_INLINE_OPT QData Vtiny_alu___024root___change_request_1(Vtiny_alu___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtiny_alu__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtiny_alu___024root___change_request_1\n"); );
    // Body
    // Change detection
    QData __req = false;  // Logically a bool
    return __req;
}

#ifdef VL_DEBUG
void Vtiny_alu___024root___eval_debug_assertions(Vtiny_alu___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtiny_alu__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtiny_alu___024root___eval_debug_assertions\n"); );
    // Body
    if (VL_UNLIKELY((vlSelf->a & 0xf0U))) {
        Verilated::overWidthError("a");}
    if (VL_UNLIKELY((vlSelf->b & 0xf0U))) {
        Verilated::overWidthError("b");}
    if (VL_UNLIKELY((vlSelf->op & 0xf8U))) {
        Verilated::overWidthError("op");}
}
#endif  // VL_DEBUG
