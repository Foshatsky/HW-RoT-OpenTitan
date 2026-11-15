#!/usr/bin/env python3
"""
Trace Analyzer for RISC-V Instruction Traces.
Generates a simulated trace and analyzes it for:
  - Pipeline hazards (RAW, WAW, WAR)
  - Branch mispredictions
  - Cache misses (simulated)
  - Performance metrics (CPI, IPC)
"""

import csv
import random
import sys
from collections import defaultdict
from io import StringIO

random.seed(42)
TRACE_LINES = 100
CACHE_MISS_RATE = 0.20
BRANCH_MISPREDICT_RATE = 0.15


def generate_trace(num_insts=TRACE_LINES):
    """Generate a synthetic RISC-V instruction trace."""
    regs = [f"x{i}" for i in range(32)]
    instructions = []
    pc = 0x0000
    reg_state = {r: 0 for r in regs}

    ops = [
        ("add", "R"),
        ("sub", "R"),
        ("addi", "I"),
        ("xori", "I"),
        ("ori", "I"),
        ("andi", "I"),
        ("slli", "I"),
        ("lw", "M"),
        ("sw", "M"),
        ("beq", "B"),
        ("bne", "B"),
        ("blt", "B"),
        ("bge", "B"),
    ]

    for cycle in range(num_insts):
        op, typ = random.choice(ops)

        rd = random.choice(regs) if typ in ("R", "I") else "x0"
        rs1 = random.choice(regs)
        rs2 = random.choice(regs) if typ == "R" else "x0"
        imm12 = random.randint(0, 63)
        mem_addr = "-"

        inst_str = f"{op}"
        if typ == "R":
            inst_str += f" {rd},{rs1},{rs2}"
        elif typ == "I":
            inst_str += f" {rd},{rs1},{imm12}"
        elif typ == "M":
            inst_str += f" {rd if op == 'lw' else rs2},{imm12}({rs1})"
            mem_addr = f"0x{pc + imm12:04x}"
            if op == "sw":
                rd = "x0"  # sw has no rd
        elif typ == "B":
            inst_str += f" {rs1},{rs2},target"
            rd = "x0"

        # Simulate branch target for trace readability
        if typ == "B":
            mem_addr = f"0x{pc + 8 + imm12:04x}"

        instructions.append({
            "cycle": cycle,
            "pc": f"0x{pc:04x}",
            "instruction": inst_str,
            "rd": rd,
            "rs1": rs1,
            "rs2": rs2,
            "mem_addr": mem_addr,
            "op_type": typ,
            "op_name": op,
        })

        # Update register state for dependency tracking
        if typ in ("R", "I") and rd != "x0":
            reg_state[rd] += 1

        pc += 4  # simple sequential (ignoring branches for trace gen)

    return instructions


def detect_hazards(trace):
    """Detect RAW, WAW, WAR hazards in the trace."""
    last_write = {}
    last_read = {}
    hazards = defaultdict(list)

    for i, inst in enumerate(trace):
        rd = inst["rd"]
        rs1 = inst["rs1"]
        rs2 = inst["rs2"]
        op_name = inst["op_name"]

        # RAW: read after write to same register
        for rs in [rs1, rs2]:
            if rs != "x0" and rs in last_write:
                prev = last_write[rs]
                if i - prev <= 3:  # within 3 instructions = pipeline hazard
                    hazards["RAW"].append({
                        "cycle": inst["cycle"],
                        "inst": inst["instruction"],
                        "reg": rs,
                        "distance": i - prev,
                        "prev_inst": trace[prev]["instruction"],
                        "prev_cycle": trace[prev]["cycle"],
                    })
                    if i - prev == 1:
                        inst["_has_raw"] = True

        # WAW: write after write to same register
        if rd != "x0" and rd in last_write:
            prev = last_write[rd]
            if i - prev <= 3:
                hazards["WAW"].append({
                    "cycle": inst["cycle"],
                    "inst": inst["instruction"],
                    "reg": rd,
                    "distance": i - prev,
                    "prev_inst": trace[prev]["instruction"],
                    "prev_cycle": trace[prev]["cycle"],
                })

        # WAR: write after read to same register
        if rd != "x0" and rd in last_read:
            prev = last_read[rd]
            if i - prev <= 3:
                hazards["WAR"].append({
                    "cycle": inst["cycle"],
                    "inst": inst["instruction"],
                    "reg": rd,
                    "distance": i - prev,
                    "prev_inst": trace[prev]["instruction"],
                    "prev_cycle": trace[prev]["cycle"],
                })

        # Update tracking state
        if rd != "x0":
            last_write[rd] = i
        for rs in [rs1, rs2]:
            if rs != "x0":
                last_read[rs] = i

    return hazards


def detect_branch_mispredictions(trace):
    """Simulate branch mispredictions (random)."""
    mispredictions = []
    for inst in trace:
        if inst["op_type"] == "B":
            if random.random() < BRANCH_MISPREDICT_RATE:
                mispredictions.append({
                    "cycle": inst["cycle"],
                    "inst": inst["instruction"],
                    "taken": random.choice([True, False]),
                })
    return mispredictions


def detect_cache_misses(trace):
    """Simulate cache misses for memory operations."""
    misses = []
    for inst in trace:
        if inst["op_type"] == "M":
            if random.random() < CACHE_MISS_RATE:
                misses.append({
                    "cycle": inst["cycle"],
                    "inst": inst["instruction"],
                    "addr": inst["mem_addr"],
                    "type": "load" if inst["op_name"] == "lw" else "store",
                    "penalty": random.randint(10, 30),
                })
    return misses


def compute_performance(trace, hazards, mispredictions, cache_misses):
    """Compute CPI and IPC."""
    total_insts = len(trace)
    stall_cycles = 0

    for h in hazards.get("RAW", []):
        stall_cycles += max(0, 2 - h["distance"])
    for h in hazards.get("WAW", []):
        stall_cycles += 1
    for h in hazards.get("WAR", []):
        stall_cycles += 1

    misp_penalty = len(mispredictions) * 3
    cache_penalty = sum(c["penalty"] for c in cache_misses)

    total_cycles = total_insts + stall_cycles + misp_penalty + cache_penalty
    cpi = total_cycles / total_insts if total_insts else 0
    ipc = total_insts / total_cycles if total_cycles else 0

    return {
        "total_instructions": total_insts,
        "total_cycles": total_cycles,
        "stall_cycles": stall_cycles,
        "mispredict_penalty_cycles": misp_penalty,
        "cache_penalty_cycles": cache_penalty,
        "cpi": round(cpi, 3),
        "ipc": round(ipc, 3),
    }


def print_analysis(trace, hazards, mispredictions, cache_misses, perf, file):
    """Print a full analysis report to the given file object."""

    def emit(line=""):
        print(line, file=file)

    emit("=" * 72)
    emit("  RISC-V TRACE DEBUGGING ANALYSIS REPORT")
    emit("=" * 72)
    emit(f"Trace length: {len(trace)} instructions")
    emit(f"Trace format: cycle, pc, instruction, rd, rs1, rs2, mem_addr")
    emit()

    # --- First 10 trace entries ---
    emit("--- FIRST 10 TRACE ENTRIES ---")
    emit(f"{'cycle':>5} {'pc':<8} {'instruction':<24} {'rd':<4} {'rs1':<4} {'rs2':<4} {'mem_addr':<10}")
    emit("-" * 65)
    for inst in trace[:10]:
        emit(f"{inst['cycle']:>5} {inst['pc']:<8} {inst['instruction']:<24} {inst['rd']:<4} {inst['rs1']:<4} {inst['rs2']:<4} {inst['mem_addr']:<10}")
    emit()

    # --- Pipeline Hazards ---
    emit("--- PIPELINE HAZARDS ---")
    for htype in ("RAW", "WAW", "WAR"):
        hlist = hazards.get(htype, [])
        emit(f"\n{htype} hazards (count: {len(hlist)}):")
        if hlist:
            emit(f"  {'Cycle':<6} {'Inst':<24} {'Reg':<4} {'Dist':<5} {'Prev Inst':<24} {'Prev Cycle':<6}")
            emit("  " + "-" * 75)
            for h in hlist[:10]:
                emit(f"  {h['cycle']:<6} {h['inst']:<24} {h['reg']:<4} {h['distance']:<5} {h['prev_inst']:<24} {h['prev_cycle']:<6}")
            if len(hlist) > 10:
                emit(f"  ... and {len(hlist) - 10} more {htype} hazards")
        else:
            emit("  (none detected)")

    emit()

    # --- Branch Mispredictions ---
    emit("--- BRANCH MISPREDICTIONS ---")
    emit(f"Count: {len(mispredictions)}")
    if mispredictions:
        emit(f"  {'Cycle':<6} {'Inst':<24} {'Taken':<6}")
        emit("  " + "-" * 40)
        for m in mispredictions[:5]:
            emit(f"  {m['cycle']:<6} {m['inst']:<24} {str(m['taken']):<6}")
        if len(mispredictions) > 5:
            emit(f"  ... and {len(mispredictions) - 5} more")
    emit()

    # --- Cache Misses ---
    emit("--- CACHE MISSES ---")
    emit(f"Count: {len(cache_misses)} (simulated miss rate: {CACHE_MISS_RATE:.0%})")
    if cache_misses:
        emit(f"  {'Cycle':<6} {'Inst':<24} {'Addr':<10} {'Type':<6} {'Penalty':<8}")
        emit("  " + "-" * 54)
        for c in cache_misses[:5]:
            emit(f"  {c['cycle']:<6} {c['inst']:<24} {c['addr']:<10} {c['type']:<6} {c['penalty']:<8}")
        if len(cache_misses) > 5:
            emit(f"  ... and {len(cache_misses) - 5} more")
    emit()

    # --- Performance Metrics ---
    emit("--- PERFORMANCE METRICS ---")
    emit(f"  Total instructions:       {perf['total_instructions']}")
    emit(f"  Total clock cycles:       {perf['total_cycles']}")
    emit(f"  Hazard stall cycles:      {perf['stall_cycles']}")
    emit(f"  Mispredict penalty:       {perf['mispredict_penalty_cycles']}")
    emit(f"  Cache miss penalty:       {perf['cache_penalty_cycles']}")
    emit(f"  CPI (Cycles / Instruction): {perf['cpi']}")
    emit(f"  IPC (Instructions / Cycle): {perf['ipc']}")
    emit()

    # --- Summary ---
    emit("--- SUMMARY ---")
    if perf["cpi"] > 1.5:
        emit("  WARNING: CPI > 1.5 indicates significant pipeline stalls.")
    elif perf["cpi"] > 1.0:
        emit("  NOTE: CPI > 1.0, some pipeline inefficiency detected.")
    else:
        emit("  Good CPI value, pipeline is efficient.")

    if len(hazards.get("RAW", [])) > 10:
        emit("  WARNING: High RAW hazard count — consider forwarding logic.")
    if len(cache_misses) > 15:
        emit("  WARNING: High cache miss rate — consider larger/tighter cache.")

    emit("=" * 72)


def main():
    output_file = "output.log"

    with open(output_file, "w", encoding="utf-8") as f:
        trace = generate_trace()
        hazards = detect_hazards(trace)
        mispredictions = detect_branch_mispredictions(trace)
        cache_misses = detect_cache_misses(trace)
        perf = compute_performance(trace, hazards, mispredictions, cache_misses)

        print_analysis(trace, hazards, mispredictions, cache_misses, perf, file=f)

        # Also write trace CSV to a separate section
        print("\n=== FULL TRACE CSV ===\n", file=f)
        writer = csv.DictWriter(
            f,
            fieldnames=["cycle", "pc", "instruction", "rd", "rs1", "rs2", "mem_addr"],
        )
        writer.writeheader()
        for inst in trace:
            row = {
                "cycle": inst["cycle"],
                "pc": inst["pc"],
                "instruction": inst["instruction"],
                "rd": inst["rd"],
                "rs1": inst["rs1"],
                "rs2": inst["rs2"],
                "mem_addr": inst["mem_addr"],
            }
            writer.writerow(row)

    # Also print to stdout
    trace = generate_trace()
    hazards = detect_hazards(trace)
    mispredictions = detect_branch_mispredictions(trace)
    cache_misses = detect_cache_misses(trace)
    perf = compute_performance(trace, hazards, mispredictions, cache_misses)
    print_analysis(trace, hazards, mispredictions, cache_misses, perf, file=sys.stdout)

    print(f"\nFull output written to {output_file}")


if __name__ == "__main__":
    main()
