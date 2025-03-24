# RISC-V Assembler & Simulator

## Overview

This project consists of two phases:

1. **Phase 1 (Assembler)**: Converts RISC-V assembly code (`input.asm`) into machine code (`output.mc`).
2. **Phase 2 (Simulator)**: Implements a functional simulator that executes machine code (`output.mc`) step-by-step, following the RISC-V 32-bit instruction execution pipeline.

---

## Phase 1: Assembler

The assembler reads an assembly source file (`input.asm`) and produces a machine code output file (`output.mc`) that contains:

- **Code Segment**: Each instruction’s address, machine code in hex, and a bit-field breakdown comment.
- **Data Segment**: Initial memory contents defined by directives (e.g., `.data`, `.byte`, `.word`).

### Supported Instructions
The assembler supports the following **RISC-V instructions**:

- **R-type**: `add, sub, and, or, xor, sll, srl, sra, slt, mul, div, rem`
- **I-type**: `addi, andi, ori, lb, lh, lw, ld, jalr`
- **S-type**: `sb, sh, sw, sd`
- **SB-type**: `beq, bne, bge, blt`
- **U-type**: `lui, auipc`
- **UJ-type**: `jal`

### Assembler Directives
- `.text`, `.data`
- `.byte`, `.half`, `.word`, `.dword`, `.asciz`

---

## Phase 2: Simulator

The simulator reads `output.mc` (generated from Phase 1) and executes instructions using a **five-stage pipeline execution model**:

1. **Fetch**: Reads the next instruction using the program counter (`PC`).
2. **Decode**: Decodes the instruction into opcode, registers, and immediate values.
3. **Execute**: Performs the ALU operation (addition, subtraction, bitwise operations, etc.).
4. **Memory Access**: Loads/stores values from/to memory.
5. **Register Writeback**: Writes the result back to the destination register.

Each instruction execution updates **registers, memory, and PC**, and provides detailed messages at each stage.

### New Features in Phase 2
✅ **Clock Cycle Tracking**: The simulator introduces a clock variable that increments for every instruction executed.  
✅ **Step-by-Step Execution**: Users can execute instructions one by one (`Next`) or run the entire program (`Run`).  
✅ **GUI Integration**: A frontend displays register values, memory state, and output logs dynamically.  
✅ **Memory Dumping**: The simulator updates `data.mc`, `stack.mc`, and `instruction.mc` after every execution.  
✅ **Exit Instruction**: A special instruction stops execution and dumps the final memory state.  

---

## File Structure

```
📂 RISC-V-Assembler
│── 📂 backend/              # Backend (Simulator)
│   │── server.js           # Node.js backend to communicate with frontend
│   │── simulator.cpp       # RISC-V functional simulator
│   │── package.json        # Dependencies for backend
│   └── ...
│
│── 📂 frontend/             # Frontend (GUI)
│   │── src/                # React application source code
│   │── App.tsx             # Main frontend component
│   │── package.json        # Dependencies for frontend
│   └── ...
│
│── input.asm               # Sample assembly input
│── output.mc               # Assembler-generated machine code
│── data.mc                 # Data segment memory dump
│── stack.mc                # Stack segment memory dump
│── instruction.mc          # Instruction memory dump
│── README.md               # This file
```

---

## Build & Execution

### Running the Assembler (Phase 1)
1. Open a terminal and navigate to the project directory.
2. Compile the assembler:
   ```
   g++ -std=c++11 -Iinclude src/*.cpp -o assembler
   ```
3. Run the assembler:
   ```
   ./assembler
   ```
   This converts `input.asm` into `output.mc`.

---

### Running the Simulator (Phase 2)
#### 1️⃣ **Option 1: CLI Mode**
Run the simulator using:
```
g++ simulator.cpp -o simulator
./simulator input.mc data.mc stack.mc instruction.mc
```
- Enter **`N`** to execute the next instruction step-by-step.
- Enter **`R`** to execute all remaining instructions at once.
- Enter **`E`** to exit the simulation.

---

#### 2️⃣ **Option 2: GUI Mode**
The GUI provides an interactive way to run and visualize the simulation.

📌 **Setup:**
1. Install backend dependencies:
   ```
   cd backend
   npm install
   ```
2. Install frontend dependencies:
   ```
   cd ../frontend
   npm install
   ```
3. Start the backend server:
   ```
   cd ../backend
   node server.js
   ```
4. Start the frontend application:
   ```
   cd ../frontend
   npm start
   ```

---

## Usage

1. Write RISC-V assembly code in `input.asm`:
   ```assembly
   .data
   arrayA: .byte 10,20,30,40
   arrayB: .word 0xDEADBEEF

   .text
   main:
       addi x1, x0, 5
       beq x1, x0, end
       add x0, x0, x0
   end:
       jal x0, end
   ```

2. Run the assembler and simulator.

3. The **GUI** displays:
   - **Registers** (x0-x31)
   - **Data & Stack Memory**
   - **Execution Logs** (showing each step: Fetch, Decode, Execute, Memory, WB)

---

## Supported Syntax & Limitations

✔️ **Supported Instruction Formats**
- **R-type**: `add x1, x2, x3`
- **I-type**: `addi x1, x2, 10`, `lb x10, 0(x2)`, `jalr x1, 0(x5)`
- **S-type**: `sb x6, 4(x7)`
- **SB-type**: `beq x1, x2, label`
- **U-type**: `lui x5, 0x12345`, `auipc x6, 0xABCD`
- **UJ-type**: `jal x1, label`

❌ **Known Limitations**
- **Pseudoinstructions (e.g., li, mv) are NOT supported**.
- **Floating-point instructions are NOT implemented**.
- **Branch and jump offsets** are computed based on `PC + 4`.
- **Runtime memory updates are NOT simulated beyond store/load operations**.

---

## Contact & Support

For any issues, contributions, or queries, contact:

📩 **Developers:**
- **Ashutosh Rajput** – *2023csb1289@iitrpr.ac.in*
- **Aryan Sodhi** – *2023csb1288@iitrpr.ac.in*
- **Bhavya Rao** – *2023csb1291@iitrpr.ac.in*

---

This README now includes Phase 2, explaining the simulator features, execution process, and GUI integration. 🚀

