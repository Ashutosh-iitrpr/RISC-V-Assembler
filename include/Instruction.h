#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>
#include <vector>
#include <cstdint>

struct AsmLine {
    bool isInstruction = false;          // true if this line is an instruction
    bool isDirective   = false;          // true if it is a directive (.text, .data, etc.)
    std::string label;                   // label (if any)
    std::string directive;               // directive name (e.g. ".word", ".byte")
    std::vector<std::string> tokens;     // tokens for instruction or data values
    std::string originalLine;            // the original source line
    uint32_t address = 0;                // memory address for instructions/data
};

struct InstructionLine {
    uint32_t address;
    std::string mnemonic;
    std::vector<std::string> operands;   // e.g. x1, x2, x3 or x5, x6, 10
    std::string originalLine;
};

#endif // INSTRUCTION_H
