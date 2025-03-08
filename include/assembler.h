#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include "Instruction.h"

class Assembler {
public:
    Assembler(const std::string &inputFile, const std::string &outputFile);
    void assemble();
private:
    std::string inputFilename;
    std::string outputFilename;
    std::vector<AsmLine> lines;
    std::vector<InstructionLine> textInstructions;
    std::unordered_map<std::string, uint32_t> symbolTable;
    std::map<uint32_t, uint8_t> dataMemory;
    uint32_t currentTextAddr;
    uint32_t currentDataAddr;
    int currentSection; // Use Section enum values (UNDEF, TEXT, DATA)

    void pass1();
    void pass2();
};

#endif // ASSEMBLER_H
