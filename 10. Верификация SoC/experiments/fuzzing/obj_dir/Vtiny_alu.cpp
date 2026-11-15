// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Model implementation (design independent parts)

#include "Vtiny_alu.h"
#include "Vtiny_alu__Syms.h"

//============================================================
// Constructors

Vtiny_alu::Vtiny_alu(VerilatedContext* _vcontextp__, const char* _vcname__)
    : vlSymsp{new Vtiny_alu__Syms(_vcontextp__, _vcname__, this)}
    , a{vlSymsp->TOP.a}
    , b{vlSymsp->TOP.b}
    , op{vlSymsp->TOP.op}
    , result{vlSymsp->TOP.result}
    , carry_flag{vlSymsp->TOP.carry_flag}
    , rootp{&(vlSymsp->TOP)}
{
}

Vtiny_alu::Vtiny_alu(const char* _vcname__)
    : Vtiny_alu(nullptr, _vcname__)
{
}

//============================================================
// Destructor

Vtiny_alu::~Vtiny_alu() {
    delete vlSymsp;
}

//============================================================
// Evaluation loop

void Vtiny_alu___024root___eval_initial(Vtiny_alu___024root* vlSelf);
void Vtiny_alu___024root___eval_settle(Vtiny_alu___024root* vlSelf);
void Vtiny_alu___024root___eval(Vtiny_alu___024root* vlSelf);
QData Vtiny_alu___024root___change_request(Vtiny_alu___024root* vlSelf);
#ifdef VL_DEBUG
void Vtiny_alu___024root___eval_debug_assertions(Vtiny_alu___024root* vlSelf);
#endif  // VL_DEBUG
void Vtiny_alu___024root___final(Vtiny_alu___024root* vlSelf);

static void _eval_initial_loop(Vtiny_alu__Syms* __restrict vlSymsp) {
    vlSymsp->__Vm_didInit = true;
    Vtiny_alu___024root___eval_initial(&(vlSymsp->TOP));
    // Evaluate till stable
    int __VclockLoop = 0;
    QData __Vchange = 1;
    do {
        VL_DEBUG_IF(VL_DBG_MSGF("+ Initial loop\n"););
        Vtiny_alu___024root___eval_settle(&(vlSymsp->TOP));
        Vtiny_alu___024root___eval(&(vlSymsp->TOP));
        if (VL_UNLIKELY(++__VclockLoop > 100)) {
            // About to fail, so enable debug to see what's not settling.
            // Note you must run make with OPT=-DVL_DEBUG for debug prints.
            int __Vsaved_debug = Verilated::debug();
            Verilated::debug(1);
            __Vchange = Vtiny_alu___024root___change_request(&(vlSymsp->TOP));
            Verilated::debug(__Vsaved_debug);
            VL_FATAL_MT("tiny_alu.v", 1, "",
                "Verilated model didn't DC converge\n"
                "- See https://verilator.org/warn/DIDNOTCONVERGE");
        } else {
            __Vchange = Vtiny_alu___024root___change_request(&(vlSymsp->TOP));
        }
    } while (VL_UNLIKELY(__Vchange));
}

void Vtiny_alu::eval_step() {
    VL_DEBUG_IF(VL_DBG_MSGF("+++++TOP Evaluate Vtiny_alu::eval_step\n"); );
#ifdef VL_DEBUG
    // Debug assertions
    Vtiny_alu___024root___eval_debug_assertions(&(vlSymsp->TOP));
#endif  // VL_DEBUG
    // Initialize
    if (VL_UNLIKELY(!vlSymsp->__Vm_didInit)) _eval_initial_loop(vlSymsp);
    // Evaluate till stable
    int __VclockLoop = 0;
    QData __Vchange = 1;
    do {
        VL_DEBUG_IF(VL_DBG_MSGF("+ Clock loop\n"););
        Vtiny_alu___024root___eval(&(vlSymsp->TOP));
        if (VL_UNLIKELY(++__VclockLoop > 100)) {
            // About to fail, so enable debug to see what's not settling.
            // Note you must run make with OPT=-DVL_DEBUG for debug prints.
            int __Vsaved_debug = Verilated::debug();
            Verilated::debug(1);
            __Vchange = Vtiny_alu___024root___change_request(&(vlSymsp->TOP));
            Verilated::debug(__Vsaved_debug);
            VL_FATAL_MT("tiny_alu.v", 1, "",
                "Verilated model didn't converge\n"
                "- See https://verilator.org/warn/DIDNOTCONVERGE");
        } else {
            __Vchange = Vtiny_alu___024root___change_request(&(vlSymsp->TOP));
        }
    } while (VL_UNLIKELY(__Vchange));
}

//============================================================
// Invoke final blocks

void Vtiny_alu::final() {
    Vtiny_alu___024root___final(&(vlSymsp->TOP));
}

//============================================================
// Utilities

VerilatedContext* Vtiny_alu::contextp() const {
    return vlSymsp->_vm_contextp__;
}

const char* Vtiny_alu::name() const {
    return vlSymsp->name();
}
