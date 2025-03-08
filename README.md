# RISC-V-Assembler

OVERVIEW
--------
This project implements a simple assembler for a subset of the
RISC-V 32-bit Instruction Set Architecture (ISA). It reads an assembly
source file (input.asm) and produces a machine code output file (output.mc)
that contains two segments:
  • Code Segment: Each instruction’s address, machine code in hex, and a
    bit-field breakdown comment.
  • Data Segment: The initial contents of memory locations defined by
    directives (e.g., .data, .byte, .word, etc.).

The assembler supports:
  • R-type instructions: add, sub, and, or, xor, sll, srl, sra, slt, mul, div, rem
  • I-type instructions: addi, andi, ori, lb, lh, lw, ld, jalr
  • S-type instructions: sb, sh, sw, sd
  • SB-type instructions: beq, bne, bge, blt
  • U-type instructions: lui, auipc
  • UJ-type instructions: jal
  • Assembler directives: .text, .data, .byte, .half, .word, .dword, .asciz

FILE STRUCTURE
--------------
The main folder contains:
  • input.asm          - The assembly source file.
  • output.mc          - The generated machine code file (after assembly).
  • src/               - Source files for the assembler.
  • include/           - Header files defining instruction types, utilities, and constants.

Contents of the "include/" directory:
  • IType.h
  • RType.h
  • SType.h
  • UType.h
  • SBType.h
  • UJType.h
  • Instruction.h
  • assembler.h
  • Utils.h
  • constants.h

Contents of the "src/" directory:
  • main.cpp
  • assembler.cpp
  • IType.cpp
  • RType.cpp
  • SType.cpp
  • UType.cpp
  • SBType.cpp
  • UJType.cpp
  • Utils.cpp

BUILD & EXECUTION
-----------------
1. Open a terminal in the project’s main folder.
2. Compile the assembler using the following command:
     g++ -std=c++11 -Iinclude src/*.cpp -o assembler
3. Run the assembler:
     ./assembler
4. The assembler reads "input.asm" and generates "output.mc", which includes:
   - A code segment with each instruction’s address, its machine code, and a bit breakdown.
   - A data segment listing the initial contents for all data locations.

USAGE
-----
1. Prepare your assembly source code in "input.asm" following standard RISC-V syntax.
   For example:

     .data
     arrayA: .byte 10,20,30,40
     arrayB: .word 0xDEADBEEF

     .text
     main:
         addi x1, x0, 5
         beq x1, x0, end
         add  x0, x0, x0
     end:
         jal  x0, end

2. Run the assembler (as described above).
3. Inspect "output.mc" to verify that the machine code and data segments are correct.

SUPPORTED SYNTAX & LIMITATIONS
------------------------------
Supported Instruction Formats:
  - R-type:    e.g., add x1, x2, x3
  - I-type:    e.g., addi x1, x2, 10 | lb x10, 0(x2) | jalr x1, 0(x5)
  - S-type:    e.g., sb x6, 4(x7)
  - SB-type:   e.g., beq x1, x2, label
  - U-type:    e.g., lui x5, 0x12345 | auipc x6, 0xABCD
  - UJ-type:   e.g., jal x1, label

Supported Directives:
  - .text, .data
  - .byte, .half, .word, .dword, .asciz

Known Limitations:
  • Pseudoinstructions are not supported.
  • Floating-point instructions are not supported.
  • Branch and jump offsets are computed from the next instruction address (PC + 4).
  • The assembler does not execute the code, so runtime memory updates are not simulated.

CONTACT & SUPPORT
-----------------
For questions, issues, or contributions, please contact:
   [Ashutosh Rajput] - [2023csb1289@iitrpr.ac.in]
   [Aryan Sodhi] - [2023csb1288@iitrpr.ac.in]
   [Bhavya Rao] - [2023csb1291@iitrpr.ac.in]

