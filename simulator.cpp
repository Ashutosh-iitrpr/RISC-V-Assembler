#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>

// =====================================================================
// Global CPU State Definitions
// =====================================================================
static const int NUM_REGS = 32;
int32_t R[NUM_REGS];       // Register file (32 registers)

// Global CPU registers:
uint32_t PC = 0;           // Program Counter: current instruction address
uint32_t IR = 0;           // Instruction Register: current instruction
int32_t RA = 0;            // First operand (from register file or PC)
int32_t RB = 0;            // Second operand (set to immediate for I-type and memory ops)
int32_t RM = 0;            // Register value from rs2 (used in store data)
int32_t RZ = 0;            // ALU output (e.g., effective address)
int32_t RY = 0;            // Write-back result (selected from RZ, memory data, or return address)
int32_t MDR = 0;           // Memory Data Register: holds data fetched from memory

uint64_t clockCycle = 0;   // Clock cycle counter

// Instruction memory: maps address -> 32-bit instruction
std::map<uint32_t, uint32_t> instrMemory;

// =====================================================================
// DataSegment Class: Dynamic Data Segment
// =====================================================================
class DataSegment {
public:
    // Memory is stored per byte (address -> 8-bit value)
    std::map<uint32_t, uint8_t> memory;

    void loadFromFile(const std::string &filename) {
        std::ifstream fin(filename);
        if (!fin.is_open()) {
            std::cerr << "ERROR: Could not open file " << filename << " for data segment loading.\n";
            return;
        }
        std::string line;
        while (std::getline(fin, line)) {
            if (line.empty() || line[0]=='#')
                continue;
            std::stringstream ss(line);
            std::string addrStr, dataStr;
            ss >> addrStr >> dataStr;
            if (addrStr.empty() || dataStr.empty())
                continue;
            size_t pos = dataStr.find(',');
            if (pos != std::string::npos)
                dataStr = dataStr.substr(0, pos);
            try {
                uint32_t address = std::stoul(addrStr, nullptr, 16);
                if (address >= 0x10000000) {
                    uint32_t data = std::stoul(dataStr, nullptr, 16);
                    for (int i = 0; i < 4; i++) {
                        memory[address + i] = (data >> (8 * i)) & 0xFF;
                    }
                }
            } catch (...) {
                continue;
            }
        }
        fin.close();
    }

    void writeByte(uint32_t address, uint8_t value) {
        memory[address] = value;
    }

    void writeWord(uint32_t address, int32_t value) {
        for (int i = 0; i < 4; i++) {
            memory[address + i] = (value >> (8 * i)) & 0xFF;
        }
    }

    int32_t readWord(uint32_t address) {
        int32_t result = 0;
        for (int i = 0; i < 4; i++) {
            uint8_t b = 0;
            if (memory.find(address + i) != memory.end())
                b = memory[address + i];
            result |= (b << (8 * i));
        }
        return result;
    }

    int8_t readByte(uint32_t address) {
        if (memory.find(address) != memory.end())
            return (int8_t) memory[address];
        return 0;
    }

    void updateInputFile(const std::string &filename) {
        std::ifstream fin(filename);
        if (!fin.is_open()){
            std::cerr << "ERROR: Could not open file " << filename << " for updating.\n";
            return;
        }
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(fin, line))
            lines.push_back(line);
        fin.close();

        for (auto &l : lines) {
            if (l.empty() || l[0]=='#')
                continue;
            std::stringstream ss(l);
            std::string addrStr, dataStr;
            ss >> addrStr >> dataStr;
            if (addrStr.empty() || dataStr.empty())
                continue;
            try {
                uint32_t addr = std::stoul(addrStr, nullptr, 16);
                if (addr >= 0x10000000) {
                    int32_t word = readWord(addr);
                    std::stringstream newData;
                    newData << "0x" << std::hex << std::setw(8) << std::setfill('0') << word;
                    std::string rest;
                    std::getline(ss, rest);
                    l = addrStr + " " + newData.str() + rest;
                }
            } catch (...) {
                continue;
            }
        }
        std::ofstream fout(filename);
        if (!fout.is_open()){
            std::cerr << "ERROR: Could not open file " << filename << " for writing.\n";
            return;
        }
        for (const auto &l : lines)
            fout << l << "\n";
        fout.close();
        std::cout << "Input file data segment updated successfully.\n";
    }
};

DataSegment dataSegment;

// =====================================================================
// Utility Functions for Bit Manipulation and Sign Extension
// =====================================================================
static inline uint32_t getBits(uint32_t val, int hi, int lo) {
    uint32_t mask = (1u << (hi - lo + 1)) - 1;
    return (val >> lo) & mask;
}
static inline int32_t signExtend(uint32_t value, int bitCount) {
    int shift = 32 - bitCount;
    return (int32_t)((int32_t)(value << shift) >> shift);
}

// =====================================================================
// Decoded Instruction Structure
// =====================================================================
struct DecodedInstr {
    uint32_t opcode;
    uint32_t rd;
    uint32_t rs1;
    uint32_t rs2;    // For I-type instructions, set to 0.
    uint32_t funct3;
    uint32_t funct7; // For I-type instructions, set to 0.
    int32_t  imm;    // Sign-extended immediate.
};

// =====================================================================
// Instruction Decode Function
// =====================================================================
DecodedInstr decode(uint32_t instr) {
    DecodedInstr d{};
    d.opcode = getBits(instr, 6, 0);
    d.rd     = getBits(instr, 11, 7);
    d.funct3 = getBits(instr, 14, 12);
    d.rs1    = getBits(instr, 19, 15);
    if (d.opcode == 0x13 || d.opcode == 0x03 || d.opcode == 0x67) {
        d.rs2 = 0;
        d.funct7 = 0;
    } else {
        d.rs2    = getBits(instr, 24, 20);
        d.funct7 = getBits(instr, 31, 25);
    }
    switch(d.opcode) {
        case 0x13: // I-type ALU
        case 0x03: // I-type LOAD
        case 0x67: { // I-type JALR
            uint32_t imm12 = getBits(instr, 31, 20);
            d.imm = signExtend(imm12, 12);
        } break;
        case 0x23: { // S-type (store)
            uint32_t immHigh = getBits(instr, 31, 25);
            uint32_t immLow  = getBits(instr, 11, 7);
            uint32_t imm12   = (immHigh << 5) | immLow;
            d.imm = signExtend(imm12, 12);
        } break;
        case 0x63: { // SB-type (branch)
            uint32_t immBit12   = getBits(instr, 31, 31);
            uint32_t immBit11   = getBits(instr, 7, 7);
            uint32_t immBits10_5= getBits(instr, 30, 25);
            uint32_t immBits4_1 = getBits(instr, 11, 8);
            uint32_t immAll = (immBit12 << 12) | (immBit11 << 11) |
                              (immBits10_5 << 5) | (immBits4_1 << 1);
            d.imm = signExtend(immAll, 13);
        } break;
        case 0x37: // LUI
        case 0x17: { // AUIPC
            uint32_t imm20 = getBits(instr, 31, 12);
            d.imm = imm20 << 12;
        } break;
        case 0x6F: { // UJ-type (JAL)
            uint32_t immBit20    = getBits(instr, 31, 31);
            uint32_t immBits19_12= getBits(instr, 19, 12);
            uint32_t immBit11    = getBits(instr, 20, 20);
            uint32_t immBits10_1 = getBits(instr, 30, 21);
            uint32_t immAll = (immBit20 << 20) | (immBits19_12 << 12) |
                              (immBit11 << 11) | (immBits10_1 << 1);
            d.imm = signExtend(immAll, 21);
        } break;
        default:
            d.imm = 0;
            break;
    }
    return d;
}

// =====================================================================
// Check Termination Instruction
// =====================================================================
bool isTerminationInstr(uint32_t instr) {
    return (instr == 0x00000000);
}

// =====================================================================
// Parse the input.mc File
// =====================================================================
bool parseMCFile(const std::string &filename) {
    std::ifstream fin(filename);
    if (!fin.is_open()){
        std::cerr << "ERROR: Could not open file " << filename << "\n";
        return false;
    }
    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty() || line[0]=='#')
            continue;
        std::stringstream ss(line);
        std::string addrStr, dataStr;
        ss >> addrStr >> dataStr;
        if (addrStr.empty() || dataStr.empty())
            continue;
        size_t pos = dataStr.find(',');
        if (pos != std::string::npos)
            dataStr = dataStr.substr(0, pos);
        try {
            uint32_t address = std::stoul(addrStr, nullptr, 16);
            uint32_t data = std::stoul(dataStr, nullptr, 16);
            if (address < 0x10000000) {
                instrMemory[address] = data;
            } else {
                for (int i = 0; i < 4; i++) {
                    dataSegment.memory[address + i] = (data >> (8 * i)) & 0xFF;
                }
            }
        } catch (...) {
            std::cerr << "Parsing error on line: " << line << "\n";
            continue;
        }
    }
    fin.close();
    return true;
}

// =====================================================================
// Debug: Print Registers and Global CPU Registers
// =====================================================================
void printRegisters() {
    std::cout << "Register File:\n";
    for (int i = 0; i < NUM_REGS; i++) {
        std::cout << "R[" << std::setw(2) << i << "] = " << std::setw(10)
                  << R[i] << "   ";
        if ((i + 1) % 4 == 0)
            std::cout << "\n";
    }
    std::cout << "-------------------------------------\n";
    std::cout << "PC = 0x" << std::hex << PC << std::dec
              << "  IR = 0x" << std::hex << IR << std::dec << "\n";
    std::cout << "RA = " << RA << "  RB = " << RB << "  RM = " << RM << "\n";
    std::cout << "RZ = " << RZ << "  RY = " << RY << "  MDR = " << MDR << "\n";
    std::cout << "===========================================\n";
}

// =====================================================================
// Main Simulator Loop
// =====================================================================
int main(int argc, char* argv[]) {
    std::cout << "RISC-V Simulator Starting...\n";
   
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.mc>\n";
        return 1;
    }
   
    if (!parseMCFile(argv[1])) {
        return 1;
    }
    dataSegment.loadFromFile(argv[1]);
   
    for (int i = 0; i < NUM_REGS; i++)
        R[i] = 0;
    PC = 0;
    clockCycle = 0;
   
    std::cout << "Initial state of registers:\n";
    printRegisters();
   
    // Prompt user for input
    char userInput;
    std::cout << "Enter N for next instruction, R for remaining output, E to exit: ";
    std::cin >> userInput;
    if (userInput == 'E' || userInput == 'e') {
        std::cout << "Exiting simulation as per user request.\n";
        return 0;
    }
   
    std::cout << "Beginning simulation...\n";
    bool printRemaining = (userInput == 'R' || userInput == 'r');
    while (true) {
        std::cout << "Clock Cycle: " << clockCycle << "\n";
       
        if (instrMemory.find(PC) == instrMemory.end()) {
            std::cout << "[Fetch] No instruction at PC = 0x" << std::hex << PC
                      << ". Terminating simulation.\n";
            break;
        }
        IR = instrMemory[PC];
        std::cout << "[Fetch] PC = 0x" << std::hex << PC
                  << " IR = 0x" << IR << std::dec << "\n";
       
        if (isTerminationInstr(IR)) {
            std::cout << "[Fetch] Termination instruction encountered. Exiting simulation.\n";
            break;
        }
       
        DecodedInstr d = decode(IR);
        std::cout << "[Decode] opcode=0x" << std::hex << d.opcode
                  << " rd=" << d.rd
                  << " rs1=" << d.rs1
                  << " rs2=" << d.rs2
                  << " funct3=0x" << d.funct3
                  << " funct7=0x" << d.funct7
                  << " imm=" << std::dec << d.imm << "\n";
       
        // Operand Setup:
        RA = R[d.rs1];
        // For I-type instructions, loads, stores, JALR, and LUI, set RB to the immediate.
        if (d.opcode == 0x13 || d.opcode == 0x03 || d.opcode == 0x67 || d.opcode == 0x37 || d.opcode == 0x23) {
            RB = d.imm;
        } else if (d.opcode == 0x17) { // AUIPC
            RA = PC;
            RB = d.imm;
        } else {
            RB = R[d.rs2];
        }
        // RM holds the register file value from rs2 (used for stores).
        RM = R[d.rs2];
       
        uint32_t nextPC = PC + 4;
        RZ = 0;
        RY = 0;
       
        switch (d.opcode) {
            // R-type instructions.
            case 0x33:
                switch (d.funct3) {
                    case 0x0:
                        if (d.funct7 == 0x00) {
                            RZ = RA + RB;
                            std::cout << "[Execute] ADD: " << RA << " + " << RB << " = " << RZ << "\n";
                        } else if (d.funct7 == 0x20) {
                            RZ = RA - RB;
                            std::cout << "[Execute] SUB: " << RA << " - " << RB << " = " << RZ << "\n";
                        } else if (d.funct7 == 0x01) {
                            RZ = RA * RB;
                            std::cout << "[Execute] MUL: " << RA << " * " << RB << " = " << RZ << "\n";
                        }
                        break;
                    case 0x4:
                        if (d.funct7 == 0x00) {
                            RZ = RA ^ RB;
                            std::cout << "[Execute] XOR: " << RA << " ^ " << RB << " = " << RZ << "\n";
                        } else if (d.funct7 == 0x01) {
                            if (RB == 0) {
                                RZ = 0;
                                std::cout << "[Execute] DIV: Division by zero!\n";
                            } else {
                                RZ = RA / RB;
                                std::cout << "[Execute] DIV: " << RA << " / " << RB << " = " << RZ << "\n";
                            }
                        }
                        break;
                    case 0x6:
                        if (d.funct7 == 0x00) {
                            RZ = RA | RB;
                            std::cout << "[Execute] OR: " << RA << " | " << RB << " = " << RZ << "\n";
                        } else if (d.funct7 == 0x01) {
                            if (RB == 0) {
                                RZ = 0;
                                std::cout << "[Execute] REM: Division by zero!\n";
                            } else {
                                RZ = RA % RB;
                                std::cout << "[Execute] REM: " << RA << " % " << RB << " = " << RZ << "\n";
                            }
                        }
                        break;
                    case 0x7:
                        RZ = RA & RB;
                        std::cout << "[Execute] AND: " << RA << " & " << RB << " = " << RZ << "\n";
                        break;
                    case 0x1: {
                        int shamt = RB & 0x1F;
                        RZ = RA << shamt;
                        std::cout << "[Execute] SLL: " << RA << " << " << shamt << " = " << RZ << "\n";
                    } break;
                    case 0x2:
                        RZ = (RA < RB) ? 1 : 0;
                        std::cout << "[Execute] SLT: (" << RA << " < " << RB << ") = " << RZ << "\n";
                        break;
                    case 0x5: {
                        int shamt = RB & 0x1F;
                        if (d.funct7 == 0x00) {
                            RZ = ((uint32_t)RA) >> shamt;
                            std::cout << "[Execute] SRL: " << RA << " >> " << shamt << " = " << RZ << "\n";
                        } else if (d.funct7 == 0x20) {
                            RZ = RA >> shamt;
                            std::cout << "[Execute] SRA: " << RA << " >> " << shamt << " = " << RZ << "\n";
                        }
                        break;
                    }
                    default:
                        std::cout << "[Execute] Unimplemented R-type funct3\n";
                        break;
                }
                RY = RZ; // Write-back from ALU result for R-type.
                break;
            // I-type ALU instructions.
            case 0x13:
                switch (d.funct3) {
                    case 0x0:
                        RZ = RA + RB;
                        std::cout << "[Execute] ADDI: " << RA << " + " << RB << " = " << RZ << "\n";
                        break;
                    case 0x7:
                        RZ = RA & RB;
                        std::cout << "[Execute] ANDI: " << RA << " & " << RB << " = " << RZ << "\n";
                        break;
                    case 0x6:
                        RZ = RA | RB;
                        std::cout << "[Execute] ORI: " << RA << " | " << RB << " = " << RZ << "\n";
                        break;
                    case 0x4:
                        RZ = RA ^ RB;
                        std::cout << "[Execute] XORI: " << RA << " ^ " << RB << " = " << RZ << "\n";
                        break;
                    case 0x2:
                        RZ = (RA < RB) ? 1 : 0;
                        std::cout << "[Execute] SLTI: (" << RA << " < " << RB << ") = " << RZ << "\n";
                        break;
                    case 0x1: {
                        int shamt = RB & 0x1F;
                        RZ = RA << shamt;
                        std::cout << "[Execute] SLLI: " << RA << " << " << shamt << " = " << RZ << "\n";
                    } break;
                    case 0x5: {
                        int shamt = RB & 0x1F;
                        int topBits = (RB >> 5) & 0x7F;
                        if (topBits == 0x00) {
                            RZ = ((uint32_t)RA) >> shamt;
                            std::cout << "[Execute] SRLI: " << RA << " >> " << shamt << " = " << RZ << "\n";
                        } else if (topBits == 0x20) {
                            RZ = RA >> shamt;
                            std::cout << "[Execute] SRAI: " << RA << " >> " << shamt << " = " << RZ << "\n";
                        } else {
                            std::cout << "[Execute] Unknown I-type shift extension.\n";
                        }
                    } break;
                    default:
                        std::cout << "[Execute] Unimplemented I-type funct3\n";
                        break;
                }
                RY = RZ;
                break;
            // I-type LOAD instructions.
            case 0x03: {
                uint32_t addr = RA + d.imm;
                RZ = addr; // Effective address stored in RZ.
                switch (d.funct3) {
                    case 0x0: {
                        int8_t val = dataSegment.readByte(addr);
                        MDR = val;
                        RY = MDR;
                        std::cout << "[Execute] LB: loaded byte " << (int)val
                                  << " from 0x" << std::hex << addr << std::dec << "\n";
                    } break;
                    case 0x1: {
                        int16_t val = 0;
                        for (int i = 0; i < 2; i++) {
                            uint8_t b = 0;
                            if (dataSegment.memory.find(addr + i) != dataSegment.memory.end())
                                b = dataSegment.memory[addr + i];
                            val |= (b << (8 * i));
                        }
                        MDR = val;
                        RY = MDR;
                        std::cout << "[Execute] LH: loaded halfword " << val
                                  << " from 0x" << std::hex << addr << std::dec << "\n";
                    } break;
                    case 0x2: {
                        int32_t val = dataSegment.readWord(addr);
                        MDR = val;
                        RY = MDR;
                        std::cout << "[Execute] LW: loaded word " << val
                                  << " from 0x" << std::hex << addr << std::dec << "\n";
                    } break;
                    default:
                        std::cout << "[Execute] Unimplemented LOAD funct3\n";
                        break;
                }
            }
            break;
            // S-type STORE instructions.
            case 0x23: {
                uint32_t addr = RA + d.imm;
                RZ = addr; // Effective address stored in RZ.
                switch (d.funct3) {
                    case 0x0: {
                        int8_t toStore = (int8_t)(RM & 0xFF);
                        dataSegment.writeByte(addr, toStore);
                        std::cout << "[Execute] SB: store byte " << (int)toStore
                                  << " to 0x" << std::hex << addr << std::dec << "\n";
                    } break;
                    case 0x1: {
                        int16_t toStore = (int16_t)(RM & 0xFFFF);
                        for (int i = 0; i < 2; i++) {
                            dataSegment.writeByte(addr + i, (toStore >> (8 * i)) & 0xFF);
                        }
                        std::cout << "[Execute] SH: store halfword " << toStore
                                  << " to 0x" << std::hex << addr << std::dec << "\n";
                    } break;
                    case 0x2: {
                        dataSegment.writeWord(addr, RM);
                        std::cout << "[Execute] SW: store word " << RM
                                  << " to 0x" << std::hex << addr << std::dec << "\n";
                    } break;
                    default:
                        std::cout << "[Execute] Unimplemented STORE funct3\n";
                        break;
                }
            }
            break;
            // SB-type Branch instructions.
            case 0x63: {
                switch (d.funct3) {
                    case 0x0:
                        if (RA == RM) {
                            nextPC = PC + d.imm;
                            std::cout << "[Execute] BEQ taken: new PC = 0x"
                                      << std::hex << nextPC << std::dec << "\n";
                        } else {
                            std::cout << "[Execute] BEQ not taken.\n";
                        }
                        break;
                    case 0x1:
                        if (RA != RM) {
                            nextPC = PC + d.imm;
                            std::cout << "[Execute] BNE taken: new PC = 0x"
                                      << std::hex << nextPC << std::dec << "\n";
                        } else {
                            std::cout << "[Execute] BNE not taken.\n";
                        }
                        break;
                    case 0x4:
                        if (RA < RM) {
                            nextPC = PC + d.imm;
                            std::cout << "[Execute] BLT taken: new PC = 0x"
                                      << std::hex << nextPC << std::dec << "\n";
                        } else {
                            std::cout << "[Execute] BLT not taken.\n";
                        }
                        break;
                    case 0x5:
                        if (RA >= RM) {
                            nextPC = PC + d.imm;
                            std::cout << "[Execute] BGE taken: new PC = 0x"
                                      << std::hex << nextPC << std::dec << "\n";
                        } else {
                            std::cout << "[Execute] BGE not taken.\n";
                        }
                        break;
                    default:
                        std::cout << "[Execute] Unimplemented branch funct3.\n";
                        break;
                }
            }
            break;
            // UJ-type: JAL
            case 0x6F: {
                RZ = PC + 4; // Return address.
                nextPC = PC + d.imm;
                std::cout << "[Execute] JAL: jump to 0x" << std::hex << nextPC
                          << " and return address = 0x" << (PC + 4) << std::dec << "\n";
                RY = RZ;
            }
            break;
            // I-type: JALR
            case 0x67: {
                RZ = PC + 4; // Return address.
                {
                    uint32_t target = (uint32_t)((RA + d.imm) & ~1);
                    nextPC = target;
                    std::cout << "[Execute] JALR: jump to 0x" << std::hex << nextPC
                              << " and return address = 0x" << (PC + 4) << std::dec << "\n";
                }
                RY = RZ;
            }
            break;
            // U-type: LUI
            case 0x37: {
                RZ = d.imm;
                std::cout << "[Execute] LUI: result = 0x" << std::hex << RZ << std::dec << "\n";
                RY = RZ;
            }
            break;
            // U-type: AUIPC
            case 0x17: {
                RZ = PC + d.imm;
                std::cout << "[Execute] AUIPC: result = 0x" << std::hex << RZ << std::dec << "\n";
                RY = RZ;
            }
            break;
            default:
                std::cout << "[Execute] Unknown or unimplemented opcode: 0x"
                          << std::hex << d.opcode << std::dec << "\n";
                break;
        }
       
        // WRITEBACK Stage: Update destination register (if not x0) from RY.
        switch (d.opcode) {
            case 0x33: // R-type
            case 0x13: // I-type ALU
            case 0x17: // AUIPC
            case 0x37: // LUI
            case 0x03: // LOAD instructions
            case 0x6F: // JAL
            case 0x67: // JALR
                if (d.rd != 0) {
                    R[d.rd] = RY;
                    std::cout << "[WB] R[" << d.rd << "] updated to " << R[d.rd] << "\n";
                }
                break;
            default:
                break;
        }
       
        // Ensure register x0 remains 0.
        R[0] = 0;
        PC = nextPC;
        printRegisters();
        clockCycle++;

        if (!printRemaining) {
            // Prompt user for input
            std::cout << "Enter N for next instruction, R for remaining output, E to exit: ";
            std::cin >> userInput;
            if (userInput == 'E' || userInput == 'e') {
                std::cout << "Exiting simulation as per user request.\n";
                break;
            } else if (userInput == 'R' || userInput == 'r') {
                printRemaining = true;
            }
        }
    }
   
    std::cout << "Simulation finished after " << clockCycle << " clock cycles.\n";
    dataSegment.updateInputFile(argv[1]);
    return 0;
}
