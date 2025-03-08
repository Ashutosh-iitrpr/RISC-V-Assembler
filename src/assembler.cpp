#include "assembler.h"
#include "Utils.h"
#include "constants.h"
#include "RType.h"
#include "IType.h"
#include "SType.h"
#include "SBType.h"
#include "UType.h"
#include "UJType.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <tuple>
#include <iomanip>
#include <cstdlib>

using namespace std;

Assembler::Assembler(const string &inputFile, const string &outputFile)
    : inputFilename(inputFile), outputFilename(outputFile),
      currentTextAddr(TEXT_START), currentDataAddr(DATA_START), currentSection(UNDEF) {}

//
// Helper: encode a single instruction line. (This is adapted from your original code.)
//
uint32_t encodeInstruction(const InstructionLine &inst,
                           const unordered_map<string, uint32_t> &symbolTable,
                           string &bitBreakdown) {
    string mnemonic = toUpper(inst.mnemonic);
    const auto &op = inst.operands;
    uint32_t machineCode = 0;

    // For referencing labels.
    auto getLabelOffset = [&](const string &label, uint32_t currentPC) -> int32_t {
        auto it = symbolTable.find(label);
        if (it == symbolTable.end()) {
            cerr << "[ERROR] Label not found: " << label << endl;
            return 0;
        }
        return (int32_t)it->second - (int32_t)currentPC;
    };
    // Parse immediate or label.
    auto parseImmOrLabel = [&](const string &immstr, uint32_t currentPC) -> int32_t {
        if (symbolTable.find(immstr) != symbolTable.end())
            return getLabelOffset(immstr, currentPC);
        else
            return parseImmediate(immstr);
    };

    // R-type table.
    static unordered_map<string, tuple<uint8_t,uint8_t,uint8_t>> rTable = {
        {"ADD", {0x33, 0x0, 0x00}},
        {"SUB", {0x33, 0x0, 0x20}},
        {"AND", {0x33, 0x7, 0x00}},
        {"OR",  {0x33, 0x6, 0x00}},
        {"XOR", {0x33, 0x4, 0x00}},
        {"SLL", {0x33, 0x1, 0x00}},
        {"SRL", {0x33, 0x5, 0x00}},
        {"SRA", {0x33, 0x5, 0x20}},
        {"SLT", {0x33, 0x2, 0x00}},
        {"MUL", {0x33, 0x0, 0x01}},
        {"DIV", {0x33, 0x4, 0x01}},
        {"REM", {0x33, 0x6, 0x01}},
    };

    // I-type for addi, andi, ori.
    static unordered_map<string, pair<uint8_t,uint8_t>> iTable = {
        {"ADDI", {0x13, 0x0}},
        {"ANDI", {0x13, 0x7}},
        {"ORI",  {0x13, 0x6}},
    };

    // S-type (stores): SB, SH, SW, SD.
    static unordered_map<string, uint8_t> sTable = {
        {"SB", 0},
        {"SH", 1},
        {"SW", 2},
        {"SD", 3},
    };

    // SB-type branches: BEQ, BNE, BLT, BGE.
    static unordered_map<string, uint8_t> sbTable = {
        {"BEQ", 0},
        {"BNE", 1},
        {"BLT", 4},
        {"BGE", 5},
    };

    // R-type.
    if (rTable.find(mnemonic) != rTable.end()) {
        if (op.size() != 3)
            cerr << "[ERROR] R-type expects 3 operands\n";
        int rd  = getRegisterNumber(op[0]);
        int rs1 = getRegisterNumber(op[1]);
        int rs2 = getRegisterNumber(op[2]);
        auto [opcode, func3, func7] = rTable[mnemonic];
        machineCode = encodeRType(opcode, func3, func7, rd, rs1, rs2);
        bitBreakdown = buildBitCommentR(opcode, func3, func7, rd, rs1, rs2);
    }
    // I-type loads: LB, LH, LW, LD (opcode 0x03).
    else if (mnemonic == "LB" || mnemonic == "LH" ||
             mnemonic == "LW" || mnemonic == "LD") {
        if (op.size() != 2)
            cerr << "[ERROR] Load instruction expects 2 operands\n";
        int rd = getRegisterNumber(op[0]);
        string offsetReg = op[1];
        auto pos1 = offsetReg.find('(');
        auto pos2 = offsetReg.find(')');
        int32_t immVal = 0;
        int rs1 = 0;
        if (pos1 != string::npos && pos2 != string::npos && pos2 > pos1) {
            string immPart = offsetReg.substr(0, pos1);
            string regPart = offsetReg.substr(pos1+1, pos2 - (pos1+1));
            immVal = parseImmOrLabel(immPart, inst.address);
            rs1 = getRegisterNumber(regPart);
        } else {
            cerr << "[ERROR] Malformed load operand: " << offsetReg << endl;
        }
        uint8_t func3 = 0;
        if (mnemonic=="LB") func3 = 0;
        if (mnemonic=="LH") func3 = 1;
        if (mnemonic=="LW") func3 = 2;
        if (mnemonic=="LD") func3 = 3;
        uint8_t opcode = 0x03;
        machineCode = encodeIType(opcode, func3, rd, rs1, immVal);
        bitBreakdown = buildBitCommentI(opcode, func3, rd, rs1, immVal);
    }
    // I-type JALR.
    else if (mnemonic == "JALR") {
        if (op.size() != 2)
            cerr << "[ERROR] JALR expects 2 operands\n";
        int rd = getRegisterNumber(op[0]);
        string offsetReg = op[1];
        auto pos1 = offsetReg.find('(');
        auto pos2 = offsetReg.find(')');
        int32_t immVal = 0;
        int rs1 = 0;
        if (pos1 != string::npos && pos2 != string::npos && pos2 > pos1) {
            string immPart = offsetReg.substr(0, pos1);
            string regPart = offsetReg.substr(pos1+1, pos2 - (pos1+1));
            immVal = parseImmOrLabel(immPart, inst.address);
            rs1 = getRegisterNumber(regPart);
        } else {
            cerr << "[ERROR] Malformed JALR operand: " << offsetReg << endl;
        }
        uint8_t opcode = 0x67;
        uint8_t func3 = 0;
        machineCode = encodeIType(opcode, func3, rd, rs1, immVal);
        bitBreakdown = buildBitCommentI(opcode, func3, rd, rs1, immVal);
    }
    // I-type (ADDI, ANDI, ORI).
    else if (iTable.find(mnemonic) != iTable.end()) {
        if (op.size() != 3)
            cerr << "[ERROR] I-type (ADDI/ANDI/ORI) expects 3 operands\n";
        int rd = getRegisterNumber(op[0]);
        int rs1 = getRegisterNumber(op[1]);
        int32_t immVal = parseImmOrLabel(op[2], inst.address);
        auto [opcode, func3] = iTable[mnemonic];
        machineCode = encodeIType(opcode, func3, rd, rs1, immVal);
        bitBreakdown = buildBitCommentI(opcode, func3, rd, rs1, immVal);
    }
    // S-type (stores).
    else if (sTable.find(mnemonic) != sTable.end()) {
        if (op.size() != 2)
            cerr << "[ERROR] S-type expects 2 operands: rs2, imm(rs1)\n";
        int rs2 = getRegisterNumber(op[0]);
        string offsetReg = op[1];
        auto pos1 = offsetReg.find('(');
        auto pos2 = offsetReg.find(')');
        int32_t immVal = 0;
        int rs1 = 0;
        if (pos1 != string::npos && pos2 != string::npos && pos2 > pos1) {
            string immPart = offsetReg.substr(0, pos1);
            string regPart = offsetReg.substr(pos1+1, pos2 - (pos1+1));
            immVal = parseImmOrLabel(immPart, inst.address);
            rs1 = getRegisterNumber(regPart);
        } else {
            cerr << "[ERROR] Malformed S-type operand\n";
        }
        uint8_t func3 = sTable[mnemonic];
        uint8_t opcode = 0x23;
        machineCode = encodeSType(opcode, func3, rs1, rs2, immVal);
        bitBreakdown = buildBitCommentS(opcode, func3, rs1, rs2, immVal);
    }
    // SB-type (branches).
    else if (sbTable.find(mnemonic) != sbTable.end()) {
        if (op.size() != 3)
            cerr << "[ERROR] SB-type branch expects 3 operands: rs1, rs2, label\n";
        int rs1 = getRegisterNumber(op[0]);
        int rs2 = getRegisterNumber(op[1]);
        int32_t offset = parseImmOrLabel(op[2], inst.address);
        offset -= 4;  // Branch offset from next instruction.
        uint8_t func3 = sbTable[mnemonic];
        uint8_t opcode = 0x63;
        machineCode = encodeSBType(opcode, func3, rs1, rs2, offset);
        bitBreakdown = buildBitCommentSB(opcode, func3, rs1, rs2, offset);
    }
    // U-type: LUI and AUIPC.
    else if (mnemonic == "LUI" || mnemonic == "AUIPC") {
        if (op.size() != 2)
            cerr << "[ERROR] U-type expects 2 operands: rd, imm\n";
        int rd = getRegisterNumber(op[0]);
        int32_t immVal = parseImmOrLabel(op[1], inst.address);
        uint8_t opcode = (mnemonic == "LUI") ? 0x37 : 0x17;
        machineCode = encodeUType(opcode, rd, immVal);
        bitBreakdown = buildBitCommentU(opcode, rd, immVal);
    }
    // UJ-type: JAL.
    else if (mnemonic == "JAL") {
        if (op.size() != 2)
            cerr << "[ERROR] JAL expects 2 operands: rd, label/immediate\n";
        int rd = getRegisterNumber(op[0]);
        int32_t offset = parseImmOrLabel(op[1], inst.address);
        offset -= 4;
        uint8_t opcode = 0x6F;
        machineCode = encodeUJType(opcode, rd, offset);
        bitBreakdown = buildBitCommentUJ(opcode, rd, offset);
    }
    else {
        cerr << "[ERROR] Unknown instruction: " << mnemonic << endl;
    }

    return machineCode;
}

void Assembler::pass1() {
    ifstream fin(inputFilename);
    if (!fin.is_open()) {
        cerr << "[ERROR] Cannot open " << inputFilename << "\n";
        exit(1);
    }
    string rawLine;
    while(getline(fin, rawLine)) {
        rawLine = trim(rawLine);
        if(rawLine.empty()) continue;
        if(rawLine[0]=='#') continue;

        AsmLine asmLine;
        asmLine.originalLine = rawLine;

        // Check for a label.
        size_t colonPos = rawLine.find(':');
        if(colonPos != string::npos) {
            asmLine.label = trim(rawLine.substr(0, colonPos));
            rawLine = trim(rawLine.substr(colonPos+1));
            if(currentSection == TEXT)
                symbolTable[asmLine.label] = currentTextAddr;
            else if(currentSection == DATA)
                symbolTable[asmLine.label] = currentDataAddr;
            else
                symbolTable[asmLine.label] = currentTextAddr;
        }
        if(rawLine.empty()){
            lines.push_back(asmLine);
            continue;
        }
        // Directive or instruction?
        if(rawLine[0]=='.') {
            asmLine.isDirective = true;
            auto toks = splitTokens(rawLine);
            if(!toks.empty()){
                asmLine.directive = toks[0];
                for(size_t i=1;i<toks.size();i++){
                    asmLine.tokens.push_back(toks[i]);
                }
            }
            if(asmLine.directive==".text")
                currentSection = TEXT;
            else if(asmLine.directive==".data")
                currentSection = DATA;
            else if(asmLine.directive==".byte" ||
                    asmLine.directive==".half" ||
                    asmLine.directive==".word" ||
                    asmLine.directive==".dword" ||
                    asmLine.directive==".asciz") {
                if(currentSection != DATA) currentSection = DATA;
                if(asmLine.directive==".byte") {
                    for(auto &tk: asmLine.tokens) {
                        int val = parseImmediate(tk);
                        dataMemory[currentDataAddr] = (uint8_t)(val & 0xFF);
                        currentDataAddr += 1;
                    }
                }
                else if(asmLine.directive==".half") {
                    for(auto &tk: asmLine.tokens) {
                        int val = parseImmediate(tk);
                        dataMemory[currentDataAddr+0] = (uint8_t)((val >> 0)&0xFF);
                        dataMemory[currentDataAddr+1] = (uint8_t)((val >> 8)&0xFF);
                        currentDataAddr += 2;
                    }
                }
                else if(asmLine.directive==".word") {
                    for(auto &tk: asmLine.tokens) {
                        int32_t val = parseImmediate(tk);
                        dataMemory[currentDataAddr+0] = (uint8_t)((val >> 0)&0xFF);
                        dataMemory[currentDataAddr+1] = (uint8_t)((val >> 8)&0xFF);
                        dataMemory[currentDataAddr+2] = (uint8_t)((val >> 16)&0xFF);
                        dataMemory[currentDataAddr+3] = (uint8_t)((val >> 24)&0xFF);
                        currentDataAddr += 4;
                    }
                }
                else if(asmLine.directive==".dword") {
                    for(auto &tk: asmLine.tokens) {
                        long long val = strtoll(tk.c_str(), nullptr, 0);
                        for(int i=0;i<8;i++){
                            dataMemory[currentDataAddr+i] = (uint8_t)((val >> (8*i))&0xFF);
                        }
                        currentDataAddr += 8;
                    }
                }
                else if(asmLine.directive==".asciz") {
                    if(!asmLine.tokens.empty()){
                        string strVal = asmLine.tokens[0];
                        if(!strVal.empty() && strVal.front()=='\"')
                            strVal.erase(0,1);
                        if(!strVal.empty() && strVal.back()=='\"')
                            strVal.pop_back();
                        for(char c: strVal){
                            dataMemory[currentDataAddr++] = (uint8_t)c;
                        }
                        dataMemory[currentDataAddr++] = 0;
                    }
                }
            }
            lines.push_back(asmLine);
        } else {
            asmLine.isInstruction = true;
            if(currentSection != TEXT)
                currentSection = TEXT;
            asmLine.address = currentTextAddr;
            auto toks = splitTokens(rawLine);
            for(auto &t: toks){
                asmLine.tokens.push_back(t);
            }
            InstructionLine iLine;
            iLine.address = currentTextAddr;
            iLine.mnemonic = asmLine.tokens[0];
            for(size_t i=1;i<asmLine.tokens.size();i++){
                iLine.operands.push_back(asmLine.tokens[i]);
            }
            iLine.originalLine = asmLine.originalLine;
            textInstructions.push_back(iLine);
            currentTextAddr += CODE_STEP;
            lines.push_back(asmLine);
        }
    }
    fin.close();
}

void Assembler::pass2() {
    ofstream fout(outputFilename);
    if(!fout.is_open()){
        cerr << "[ERROR] Cannot open " << outputFilename << " for writing\n";
        exit(1);
    }
    // Code segment.
    for(auto &inst: textInstructions){
        string bitComment;
        uint32_t code = encodeInstruction(inst, symbolTable, bitComment);
        ostringstream assemblyStr;
        assemblyStr << inst.mnemonic;
        if(!inst.operands.empty()){
            assemblyStr << " " << inst.operands[0];
            for(size_t i=1;i<inst.operands.size();i++){
                assemblyStr << "," << inst.operands[i];
            }
        }
        fout << "0x" << std::hex << inst.address << " "
             << toHex32(code) << " , "
             << assemblyStr.str() << " # "
             << bitComment << "\n";
    }
    fout << "0x" << std::hex << currentTextAddr << " <END_OF_TEXT>\n\n";
    for(auto &kv: dataMemory){
        uint32_t addr = kv.first;
        uint8_t val = kv.second;
        fout << "0x" << std::hex << addr << " 0x"
             << std::setw(2) << std::setfill('0') << (unsigned)val << "\n";
    }
    fout.close();
}

void Assembler::assemble() {
    pass1();
    pass2();
    cout << "Assembly complete. See " << outputFilename << "\n";
}
