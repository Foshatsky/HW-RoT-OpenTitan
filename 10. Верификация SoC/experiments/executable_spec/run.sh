#!/bin/bash
SYSC_ROOT=../../tools/systemc/install
g++ -std=c++17 -I$SYSC_ROOT/include -L$SYSC_ROOT/lib -o tiny_cpu tiny_cpu.cpp -lsystemc -lm
LD_LIBRARY_PATH=$SYSC_ROOT/lib ./tiny_cpu 2>&1