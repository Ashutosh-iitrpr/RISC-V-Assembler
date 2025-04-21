# RISC-V Assembler & Simulator

## Overview

This project consists of three phases:

1. **Phase 1 (Assembler)**: Converts RISC-V assembly code (`input.asm`) into machine code (`output.mc`).
2. **Phase 2 (Simulator)**: Implements a functional simulator that executes machine code (`output.mc`) step-by-step, following the RISC-V 32-bit pipeline.
3. **Phase 3 (Pipelining)**: Enhances the simulator with a five-stage pipeline model, data hazard detection, stalling, and optional branching support (without data forwarding).

---

## Phase 1: Assembler

The assembler reads an assembly source file (`input.asm`) and produces a machine code output file (`output.mc`) containing:

- **Code Segment**: Each instruction’s address, machine code in hex, and a bit-field breakdown comment.
- **Data Segment**: Initial memory contents defined by directives (e.g., `.data`, `.byte`, `.word`).

### Supported Instructions
- **R-type**: `add, sub, and, or, xor, sll, srl, sra, slt, mul, div, rem`
- **I-type**: `addi, andi, ori, lb, lh, lw, ld, jalr`
- **S-type**: `sb, sh, sw, sd`
- **SB-type**: `beq, bne, bge, blt`
- **U-type**: `lui, auipc`
- **UJ-type**: `jal`

### Directives
- Section: `.text`, `.data`
- Data: `.byte`, `.half`, `.word`, `.dword`, `.asciz`

---

## Phase 2: Simulator (Functional)

The simulator reads `output.mc` and executes instructions via a **five-stage pipeline model**:

1. **Fetch**: Reads the next instruction using the program counter (PC).
2. **Decode**: Decodes opcode, registers, and immediates.
3. **Execute**: Performs ALU operations.
4. **Memory Access**: Loads from or stores to data memory.
5. **Writeback**: Writes results to the destination register.

### Features
- **Clock Cycle Tracking**: Increments a clock variable per instruction.
- **Step-by-Step & Run Modes**: Execute one instruction (`N`) or run all (`R`).
- **Memory Dumps**: Updates `data.mc`, `stack.mc`, and `instruction.mc` after each run.
- **Exit Instruction**: Halts simulation and dumps final state.

---

## Phase 3: Pipelining

This phase augments the functional simulator with pipeline control:

- **Five-Stage Pipeline**: IF, ID, EX, MEM, WB stages operate concurrently.
- **Data Hazard Detection**: Checks RAW hazards in ID against EX and MEM stages.
- **Stalling**: Freezes IF/ID and PC when hazards occur, injects bubbles in ID/EX.
- **Control Hazard Handling**: Detects branches/jumps in ID, stalls until EX, flushes wrong-path instructions, and updates PC via `PC_src` multiplexer.
- **Jump & Branch Control**: JAL/JALR flush immediately; branches wait for EX resolution.
- **No Data Forwarding**: Pipeline stalls only, no bypass paths.
- **Valid Bits and Enable Signals**: Each pipeline buffer tracks instruction validity; logic disables write when stalling.
- **Knob-Based Switching**: A `wrapper.cpp` file contains a hardcoded `knob1` flag to switch between unpipelined and pipelined modes.

### Key Signals and Behavior
- **stall_IF / stall_ID**: Freeze PC and IF/ID on hazard detection.
- **flush_IFID / flush_IDEX**: Clear pipeline registers to insert bubbles.
- **PC_src**: Select source for PC (sequential, branch, JAL, or JALR).
- **write_enable_PC**: Controls whether PC is updated in a cycle.
- **write_enable_IFID**: Blocks IF/ID stage during stall.
- **write_enable_IDEX**: Prevents advancing invalid instruction.

### Instruction Lifecycle Example (With Branch)
```text
Cycle 1: Fetch I1
Cycle 2: Fetch I2, Decode I1
Cycle 3: Fetch I3, Decode I2, Execute I1 (branch result ready)
Cycle 4: If branch taken, flush I2/I3, inject bubble in ID/EX
Cycle 5: Fetch from new PC
```

---

## File Structure

```
├── backend/
│   ├── assembler/           # Phase 1 assembler code (C++)
│   └── common/              # Shared helper code
├── frontend/               # GUI for visualization
│   └── ...
├── simulator_pip.cpp        # Pipelined simulator (Phase 3)
├── simulator_unpip.cpp      # Functional simulator (unpipelined, Phase 2)
├── wrapper.cpp              # Dispatcher
├── input.asm               # Sample assembly input (Phase 1)
├── output.mc               # Machine code for simulator
├── data.mc                 # Data memory dump
├── stack.mc                # Stack memory dump
├── instruction.mc          # Instruction memory dump
└── README.md               # This documentation
```

---

## Build & Run

### Phase 1: Assembler
```bash
# Build
g++ -std=c++11 assembler/*.cpp -Iinclude -o assembler
# Run
./assembler input.asm output.mc
```

### Phase 2 & 3: Simulator
```bash
# Build
g++ -std=c++17 wrapper.cpp simulator_unpip.cpp simulator_pip.cpp -o simulator
# Run
./simulator input.mc data.mc stack.mc instruction.mc
```

---

## Contact & Authors

- **Ashutosh Rajput** – 2023csb1289@iitrpr.ac.in
- **Aryan Sodhi**      – 2023csb1288@iitrpr.ac.in
- **Bhavya Rao**       – 2023csb1291@iitrpr.ac.in

Feel free to contribute, report issues, or suggest improvements!

