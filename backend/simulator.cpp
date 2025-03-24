#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>
#include <algorithm>  // for std::sort
#include <set>

using namespace std;
// =====================================================================
// Global CPU State
// =====================================================================
static const int NUM_REGS = 32;
int32_t R[NUM_REGS];       // Register file

uint32_t PC = 0;           // Program Counter
uint32_t IR = 0;           // Instruction Register
int32_t  RA = 0;           // Operand A
int32_t  RB = 0;           // Operand B
int32_t  RM = 0;           // Used for store data
int32_t  RZ = 0;           // ALU output
int32_t  RY = 0;           // Write-back data
int32_t  MDR= 0;           // Memory data register

uint64_t clockCycle = 0;   // Cycle counter

// =====================================================================
// Instruction Memory (< 0x10000000)
// =====================================================================
std::map<uint32_t, uint32_t> instrMemory;

// =====================================================================
// DataSegment Class (handles a map of address -> byte)
// We'll use it for both data memory and stack memory
// =====================================================================
class MemSegment {
public:
    // Each address in "memory" is one byte
    std::map<uint32_t, uint8_t> memory;

    void writeByte(uint32_t address, uint8_t value) {
        memory[address] = value;
    }

    void writeWord(uint32_t address, int32_t value) {
        for (int i = 0; i < 4; i++) {
            memory[address + i] = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
        }
    }

    int8_t readByte(uint32_t address) const {
        auto it = memory.find(address);
        if (it != memory.end()) {
            return static_cast<int8_t>(it->second);
        }
        return 0;
    }

    int32_t readWord(uint32_t address) const {
        int32_t result = 0;
        for (int i = 0; i < 4; i++) {
            auto it = memory.find(address + i);
            uint8_t b = (it != memory.end()) ? it->second : 0;
            result |= (b << (8 * i));
        }
        return result;
    }
};

// =====================================================================
// We will have two separate MemSegments for data and stack
// =====================================================================
MemSegment dataSegment;   // for addresses in [0x10000000, 0x7FFFFFFF)
MemSegment stackSegment;  // for addresses >= 0x7FFFFFFF

// =====================================================================
// Dumping memory to an .mc file
//   - Writes each 4-byte aligned address in ascending order
//   - Only writes addresses actually stored in the memory map
//   - Skips addresses outside the intended segment's range
// =====================================================================
void dumpSegmentToFile(const std::string &filename,
                       const MemSegment &seg,
                       uint32_t startAddr,
                       uint32_t endAddr /* inclusive or exclusive? */)
{
    // Open file for overwrite
    std::ofstream fout(filename);
    if (!fout.is_open()) {
        std::cerr << "ERROR: Could not open/create " << filename << "\n";
        return;
    }

    // Gather addresses from seg.memory
    std::vector<uint32_t> addresses;
    addresses.reserve(seg.memory.size());
    for (const auto &kv : seg.memory) {
        addresses.push_back(kv.first);
    }
    std::sort(addresses.begin(), addresses.end());

    // We'll write lines only for addresses in [startAddr, endAddr]
    // For the stack region, if endAddr < startAddr, we'll interpret that accordingly.
    // But here, let's just check the range as you prefer:
    std::set<uint32_t> visited;

    for (uint32_t addr : addresses) {
        if (addr < startAddr) {
            continue;
        }
        if (endAddr >= startAddr) {
            // "normal" range check
            if (addr >= endAddr) {
                continue;
            }
        } else {
            // If endAddr < startAddr, you might interpret it differently, but
            // for simplicity, let's just not do anything special.
        }

        // Only write 4-byte-aligned addresses
        if ((addr % 4) != 0) {
            continue;
        }
        // skip if we've visited it
        if (visited.count(addr) != 0) {
            continue;
        }

        // read the 32-bit word
        int32_t wordVal = seg.readWord(addr);

        fout << std::hex << "0x"
             << std::setw(8) << std::setfill('0') << addr << "  0x"
             << std::setw(8) << std::setfill('0') << static_cast<uint32_t>(wordVal)
             << std::dec << "\n";

        // Mark 4 bytes visited
        visited.insert(addr + 0);
        visited.insert(addr + 1);
        visited.insert(addr + 2);
        visited.insert(addr + 3);
    }

    fout.close();
}

// =====================================================================
// Dumping the instruction memory (which is map<uint32_t, uint32_t>)
// to instruction.mc
// =====================================================================
void dumpInstructionMemoryToFile(const std::string &filename) {
    std::ofstream fout(filename);
    if (!fout.is_open()) {
        std::cerr << "ERROR: Could not open/create " << filename << "\n";
        return;
    }

    // gather addresses
    std::vector<uint32_t> addresses;
    addresses.reserve(instrMemory.size());
    for (auto &kv : instrMemory) {
        addresses.push_back(kv.first);
    }
    std::sort(addresses.begin(), addresses.end());

    // write
    for (auto addr : addresses) {
        // each entry is already a full 32-bit instruction
        uint32_t word = instrMemory[addr];
        fout << std::hex
             << "0x" << std::setw(8) << std::setfill('0') << addr
             << "  0x" << std::setw(8) << std::setfill('0') << word
             << std::dec << "\n";
    }
    fout.close();
}

// =====================================================================
// Helper: signExtend, getBits
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
// DecodedInstr
// =====================================================================
struct DecodedInstr {
    uint32_t opcode;
    uint32_t rd;
    uint32_t rs1;
    uint32_t rs2;
    uint32_t funct3;
    uint32_t funct7;
    int32_t  imm;
};

// =====================================================================
// decode
// =====================================================================
DecodedInstr decode(uint32_t instr) {
    DecodedInstr d{};
    d.opcode = getBits(instr, 6, 0);
    d.rd     = getBits(instr, 11, 7);
    d.funct3 = getBits(instr, 14, 12);
    d.rs1    = getBits(instr, 19, 15);

    // Some opcodes ignore rs2/funct7 in decode
    if (d.opcode == 0x13 || d.opcode == 0x03 || d.opcode == 0x67) {
        d.rs2 = 0;
        d.funct7 = 0;
    } else {
        d.rs2    = getBits(instr, 24, 20);
        d.funct7 = getBits(instr, 31, 25);
    }

    // decode immediate
    switch(d.opcode) {
        case 0x13: // I-type ALU
        case 0x03: // I-type LOAD
        case 0x67: { // I-type JALR
            uint32_t imm12 = getBits(instr, 31, 20);
            d.imm = signExtend(imm12, 12);
        } break;
        case 0x23: { // S-type
            uint32_t immHigh = getBits(instr, 31, 25);
            uint32_t immLow  = getBits(instr, 11, 7);
            uint32_t imm12 = (immHigh << 5) | immLow;
            d.imm = signExtend(imm12, 12);
        } break;
        case 0x63: { // SB-type
            uint32_t immBit12    = getBits(instr, 31, 31);
            uint32_t immBit11    = getBits(instr, 7, 7);
            uint32_t immBits10_5 = getBits(instr, 30, 25);
            uint32_t immBits4_1  = getBits(instr, 11, 8);
            uint32_t immAll = (immBit12 << 12) | (immBit11 << 11)
                            | (immBits10_5 << 5) | (immBits4_1 << 1);
            d.imm = signExtend(immAll, 13);
        } break;
        case 0x37: // LUI
        case 0x17: { // AUIPC
            uint32_t imm20 = getBits(instr, 31, 12);
            d.imm = (int32_t)(imm20 << 12);
        } break;
        case 0x6F: { // UJ-type (JAL)
            uint32_t immBit20    = getBits(instr, 31, 31);
            uint32_t immBits19_12= getBits(instr, 19, 12);
            uint32_t immBit11    = getBits(instr, 20, 20);
            uint32_t immBits10_1 = getBits(instr, 30, 21);
            uint32_t immAll = (immBit20 << 20) | (immBits19_12 << 12)
                            | (immBit11 << 11) | (immBits10_1 << 1);
            d.imm = signExtend(immAll, 21);
        } break;
        default:
            d.imm = 0;
            break;
    }

    return d;
}

// =====================================================================
// isTerminationInstr
// =====================================================================
bool isTerminationInstr(uint32_t instr) {
    return (instr == 0x00000000);
}

// =====================================================================
// parseInputMC: read addresses from input.mc and distribute them
//   - <0x10000000 => instrMemory
//   - [0x10000000, 0x7FFFFFFF) => dataSegment
//   - >=0x7FFFFFFF => stackSegment
// =====================================================================
bool parseInputMC(const std::string &filename) {
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        std::cerr << "ERROR: Could not open " << filename << "\n";
        return false;
    }

    std::string line;
    while (std::getline(fin, line)) {
        // remove comments
        size_t cpos = line.find('#');
        if (cpos != std::string::npos) {
            line = line.substr(0, cpos);
        }
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string addrStr, dataStr;
        ss >> addrStr >> dataStr;
        if (dataStr[0] == '<' || dataStr[0] == 't') continue;
        if (addrStr.empty() || dataStr.empty()) {
            continue;
        }

        // remove trailing comma
        size_t commaPos = dataStr.find(',');
        if (commaPos != std::string::npos) {
            dataStr = dataStr.substr(0, commaPos);
        }

        try {
            uint32_t address = std::stoul(addrStr, nullptr, 16);
            uint32_t word    = std::stoul(dataStr, nullptr, 16);

            if (address < 0x10000000) {
                // instructions
                instrMemory[address] = word;
            }
            else if (address < 0x50000000) {
                // data
                dataSegment.writeWord(address, static_cast<int32_t>(word));
            }
            else {
                // stack
                // (we'll treat address >= 0x7FFFFFFF as stack region)
                stackSegment.writeWord(address, static_cast<int32_t>(word));
            }
        }
        catch (...) {
            std::cerr << "Parsing error on line: " << line << "\n";
            continue;
        }
    }
    fin.close();
    return true;
}

// =====================================================================
// printRegisters
// =====================================================================
void printRegisters() {
    cout << "{ \"registers\": [";
    for (int i = 0; i < NUM_REGS; i++) {
        cout << "{ \"id\": " << i << ", \"value\": " << R[i] << "}";
        if (i < NUM_REGS - 1) cout << ", ";
    }
    cout << "] }" << endl;
}


// =====================================================================
// getMemSegmentForAddress:
//   Helper to figure out which segment an address belongs to.
//   We will read/write from the correct segment in LOAD/STORE ops.
// =====================================================================
MemSegment* getMemSegmentForAddress(uint32_t addr) {
    if (addr < 0x10000000) {
        // For simplicity, let's assume we do NOT allow loads/stores to instruction memory
        // But if you wanted self-modifying code, you'd handle it.
        // We'll just return nullptr here to indicate invalid or unexpected.
        return nullptr;
    }
    else if (addr < 0x50000000) {
        return &dataSegment;
    }
    else {
        return &stackSegment;
    }
}

// =====================================================================
// main
// =====================================================================
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.mc>\n";
        return 1;
    }

    std::string inputFile = argv[1];
    if (!parseInputMC(inputFile)) {
        return 1;
    }

    // Before cycle 0, dump initial contents to:
    //   instruction.mc, data.mc, stack.mc
    dumpInstructionMemoryToFile("instruction.mc");
    dumpSegmentToFile("data.mc",  dataSegment,  0x10000000, 0x50000000);
    // We'll pick an upper bound just beyond 0x7FFFFFFF for stack
    // to capture addresses >= 0x7FFFFFFF (but 0xFFFFFFFF is typical 32-bit max)
    dumpSegmentToFile("stack.mc", stackSegment, 0x50000000, 0x7FFFFFFF);

    // Initialize registers
    for (int i = 0; i < NUM_REGS; i++) {
        R[i] = 0;
    }
    PC = 0;
    R[2] = 0x7FFFFFFC;         // x2 (sp) initialized to top of stack
    PC = 0;

    // Print initial register state
    std::cout << "Initial state (before cycle 0):\n";
    printRegisters();

    // Prompt user
    char userInput;
    std::cout << "Enter N for next, R for remainder, E to exit: ";
    std::cin >> userInput;
    if (userInput == 'E' || userInput == 'e') {
        std::cout << "Exiting at user request.\n";
        return 0;
    }
    bool runAllRemaining = (userInput == 'R' || userInput == 'r');

    std::cout << "Starting simulation...\n";

    while (true) {
        // Fetch
        std::cout << "Clock Cycle: " << clockCycle << "\n";
        auto it = instrMemory.find(PC);
        if (it == instrMemory.end()) {
            std::cout << "[Fetch] No instruction at PC=0x"
                      << std::hex << PC << ". Exiting.\n";
            break;
        }
        IR = it->second;
        std::cout << "[Fetch] PC=0x" << std::hex << PC
                  << " IR=0x" << IR << std::dec << "\n";

        // Termination?
        if (isTerminationInstr(IR)) {
            std::cout << "[Fetch] Encountered 0x00000000 => stop.\n";
            break;
        }

        // Decode
        DecodedInstr d = decode(IR);
        std::cout << "[Decode] opcode=0x" << std::hex << d.opcode
                  << " rd=" << d.rd << " rs1=" << d.rs1
                  << " rs2=" << d.rs2
                  << " funct3=0x" << d.funct3
                  << " funct7=0x" << d.funct7
                  << " imm=" << std::dec << d.imm << "\n";

        // Prepare operands
        RA = R[d.rs1];
        if (d.opcode == 0x13 || d.opcode == 0x03 || d.opcode == 0x67 ||
            d.opcode == 0x37 || d.opcode == 0x23) {
            // I-type, loads, JALR, LUI, store => RB=imm
            RB = d.imm;
        }
        else if (d.opcode == 0x17) {
            // AUIPC => RA=PC, RB=imm
            RA = PC;
            RB = d.imm;
        }
        else {
            RB = R[d.rs2];
        }
        RM = R[d.rs2];

        uint32_t nextPC = PC + 4;
        RZ = 0;
        RY = 0;

        // Execute
        switch (d.opcode) {
            // ---------------- R-type = 0x33
            case 0x33:
                switch (d.funct3) {
                    case 0x0:
                        if (d.funct7 == 0x00) {
                            // ADD
                            RZ = RA + RB;
                            std::cout << "[Execute] ADD: " << RA
                                      << " + " << RB << " => " << RZ << "\n";
                        } else if (d.funct7 == 0x20) {
                            // SUB
                            RZ = RA - RB;
                            std::cout << "[Execute] SUB: " << RA
                                      << " - " << RB << " => " << RZ << "\n";
                        } else if (d.funct7 == 0x01) {
                            // MUL
                            RZ = RA * RB;
                            std::cout << "[Execute] MUL: " << RA
                                      << " * " << RB << " => " << RZ << "\n";
                        }
                        break;
                    case 0x4:
                        if (d.funct7 == 0x00) {
                            // XOR
                            RZ = RA ^ RB;
                            std::cout << "[Execute] XOR: " << RA
                                      << " ^ " << RB << " => " << RZ << "\n";
                        } else if (d.funct7 == 0x01) {
                            // DIV
                            if (RB == 0) {
                                RZ = 0;
                                std::cout << "[Execute] DIV by zero!\n";
                            } else {
                                RZ = RA / RB;
                                std::cout << "[Execute] DIV: " << RA
                                          << "/" << RB << " => " << RZ << "\n";
                            }
                        }
                        break;
                    case 0x6:
                        if (d.funct7 == 0x00) {
                            // OR
                            RZ = RA | RB;
                            std::cout << "[Execute] OR: " << RA
                                      << " | " << RB << " => " << RZ << "\n";
                        } else if (d.funct7 == 0x01) {
                            // REM
                            if (RB == 0) {
                                RZ = 0;
                                std::cout << "[Execute] REM by zero!\n";
                            } else {
                                RZ = RA % RB;
                                std::cout << "[Execute] REM: " << RA
                                          << " % " << RB << " => " << RZ << "\n";
                            }
                        }
                        break;
                    case 0x7:
                        // AND
                        RZ = RA & RB;
                        std::cout << "[Execute] AND: " << RA
                                  << " & " << RB << " => " << RZ << "\n";
                        break;
                    case 0x1: {
                        // SLL
                        int shamt = RB & 0x1F;
                        RZ = RA << shamt;
                        std::cout << "[Execute] SLL: " << RA
                                  << " << " << shamt << " => " << RZ << "\n";
                    } break;
                    case 0x2:
                        // SLT
                        RZ = (RA < RB) ? 1 : 0;
                        std::cout << "[Execute] SLT => " << RZ << "\n";
                        break;
                    case 0x5: {
                        // SRL / SRA
                        int shamt = RB & 0x1F;
                        if (d.funct7 == 0x00) {
                            RZ = static_cast<int32_t>(
                                     static_cast<uint32_t>(RA) >> shamt);
                            std::cout << "[Execute] SRL => " << RZ << "\n";
                        } else if (d.funct7 == 0x20) {
                            RZ = RA >> shamt;
                            std::cout << "[Execute] SRA => " << RZ << "\n";
                        }
                    } break;
                    default:
                        std::cout << "[Execute] Unimplemented R-type funct3\n";
                        break;
                }
                RY = RZ;
                break;

            // ---------------- I-type ALU = 0x13
            case 0x13:
                switch (d.funct3) {
                    case 0x0:
                        // ADDI
                        RZ = RA + RB;
                        std::cout << "[Execute] ADDI => " << RZ << "\n";
                        break;
                    case 0x7:
                        // ANDI
                        RZ = RA & RB;
                        std::cout << "[Execute] ANDI => " << RZ << "\n";
                        break;
                    case 0x6:
                        // ORI
                        RZ = RA | RB;
                        std::cout << "[Execute] ORI => " << RZ << "\n";
                        break;
                    case 0x4:
                        // XORI
                        RZ = RA ^ RB;
                        std::cout << "[Execute] XORI => " << RZ << "\n";
                        break;
                    case 0x2:
                        // SLTI
                        RZ = (RA < RB) ? 1 : 0;
                        std::cout << "[Execute] SLTI => " << RZ << "\n";
                        break;
                    case 0x1: {
                        // SLLI
                        int shamt = RB & 0x1F;
                        RZ = RA << shamt;
                        std::cout << "[Execute] SLLI => " << RZ << "\n";
                    } break;
                    case 0x5: {
                        // SRLI / SRAI
                        int shamt = RB & 0x1F;
                        int topBits = (RB >> 5) & 0x7F;
                        if (topBits == 0x00) {
                            // SRLI
                            RZ = static_cast<int32_t>(
                                     static_cast<uint32_t>(RA) >> shamt);
                            std::cout << "[Execute] SRLI => " << RZ << "\n";
                        } else if (topBits == 0x20) {
                            // SRAI
                            RZ = RA >> shamt;
                            std::cout << "[Execute] SRAI => " << RZ << "\n";
                        }
                    } break;
                    default:
                        std::cout << "[Execute] Unimplemented I-type funct3\n";
                        break;
                }
                RY = RZ;
                break;

            // ---------------- LOAD = 0x03
            case 0x03: {
                uint32_t addr = RA + d.imm;
                MemSegment* seg = getMemSegmentForAddress(addr);
                if (!seg) {
                    std::cout << "[Execute] LOAD from invalid region! addr=0x"
                              << std::hex << addr << std::dec << "\n";
                    break;
                }
                RZ = addr;
                switch (d.funct3) {
                    case 0x0: { // LB
                        int8_t val = seg->readByte(addr);
                        MDR = val;
                        RY = MDR;
                        std::cout << "[Execute] LB => " << (int)val << "\n";
                    } break;
                    case 0x1: { // LH
                        int16_t val = 0;
                        val |= (seg->readByte(addr+0) & 0xFF);
                        val |= (seg->readByte(addr+1) & 0xFF) << 8;
                        MDR = val;
                        RY = MDR;
                        std::cout << "[Execute] LH => " << val << "\n";
                    } break;
                    case 0x2: { // LW
                        int32_t val = seg->readWord(addr);
                        MDR = val;
                        RY = MDR;
                        std::cout << "[Execute] LW => " << val << "\n";
                    } break;
                    default:
                        std::cout << "[Execute] Unimplemented LOAD funct3.\n";
                        break;
                }
            } break;

            // ---------------- STORE = 0x23
            case 0x23: {
                uint32_t addr = RA + d.imm;
                MemSegment* seg = getMemSegmentForAddress(addr);
                if (!seg) {
                    std::cout << "[Execute] STORE to invalid region! addr=0x"
                              << std::hex << addr << std::dec << "\n";
                    break;
                }
                RZ = addr;
                switch (d.funct3) {
                    case 0x0: { // SB
                        seg->writeByte(addr, static_cast<int8_t>(RM & 0xFF));
                        std::cout << "[Execute] SB => " << (RM & 0xFF) << "\n";
                    } break;
                    case 0x1: { // SH
                        int16_t toStore = static_cast<int16_t>(RM & 0xFFFF);
                        seg->writeByte(addr+0, (uint8_t)( toStore & 0xFF ));
                        seg->writeByte(addr+1, (uint8_t)((toStore >> 8)&0xFF));
                        std::cout << "[Execute] SH => " << toStore << "\n";
                    } break;
                    case 0x2: { // SW (Store Word)
                        seg->writeWord(addr, RM); 
                        std::cout << "[Execute] SW => " << RM << " at addr=0x" 
                                << std::hex << addr << std::dec << "\n";
                    } break;

                    default:
                        std::cout << "[Execute] Unimplemented STORE funct3.\n";
                        break;
                }
            } break;

            // ---------------- BRANCH = 0x63
            case 0x63: {
                switch (d.funct3) {
                    case 0x0: // BEQ
                        if (RA == RM) {
                            nextPC = PC + d.imm;
                            std::cout << "[Execute] BEQ => taken\n";
                        } else {
                            std::cout << "[Execute] BEQ => not taken\n";
                        }
                        break;
                    case 0x1: // BNE
                        if (RA != RM) {
                            nextPC = PC + d.imm;
                            std::cout << "[Execute] BNE => taken\n";
                        } else {
                            std::cout << "[Execute] BNE => not taken\n";
                        }
                        break;
                    case 0x4: // BLT
                        if (RA < RM) {
                            nextPC = PC + d.imm;
                            std::cout << "[Execute] BLT => taken\n";
                        } else {
                            std::cout << "[Execute] BLT => not taken\n";
                        }
                        break;
                    case 0x5: // BGE
                        if (RA >= RM) {
                            nextPC = PC + d.imm;
                            std::cout << "[Execute] BGE => taken\n";
                        } else {
                            std::cout << "[Execute] BGE => not taken\n";
                        }
                        break;
                    default:
                        std::cout << "[Execute] Unimplemented branch funct3.\n";
                        break;
                }
            } break;

            // ---------------- JAL = 0x6F
            case 0x6F: {
                RZ = PC + 4;
                nextPC = PC + d.imm;
                RY = RZ;
                std::cout << "[Execute] JAL => nextPC=0x"
                          << std::hex << nextPC << std::dec << "\n";
            } break;

            // ---------------- JALR = 0x67
            case 0x67: {
                RZ = PC + 4;
                uint32_t target = (RA + d.imm) & ~1U;
                nextPC = target;
                RY = RZ;
                std::cout << "[Execute] JALR => nextPC=0x"
                          << std::hex << nextPC << std::dec << "\n";
            } break;

            // ---------------- LUI = 0x37
            case 0x37: {
                RZ = d.imm;
                RY = RZ;
                std::cout << "[Execute] LUI => " << RZ << "\n";
            } break;

            // ---------------- AUIPC = 0x17
            case 0x17: {
                RZ = PC + d.imm;
                RY = RZ;
                std::cout << "[Execute] AUIPC => " << RZ << "\n";
            } break;

            default:
                std::cout << "[Execute] Unimplemented opcode=0x"
                          << std::hex << d.opcode << std::dec << "\n";
                break;
        }

        // Writeback
        switch (d.opcode) {
            case 0x33: // R-type
            case 0x13: // I-type ALU
            case 0x17: // AUIPC
            case 0x37: // LUI
            case 0x03: // LOAD
            case 0x6F: // JAL
            case 0x67: // JALR
                if (d.rd != 0) {
                    R[d.rd] = RY;
                    std::cout << "[WB] R[" << d.rd << "] => " << R[d.rd] << "\n";
                }
                break;
            default:
                // no WB for store/branch
                break;
        }

        R[0] = 0; // x0 always 0
        PC = nextPC;

        printRegisters();

        // After each cycle, update the 3 .mc files
        dumpInstructionMemoryToFile("instruction.mc");
        dumpSegmentToFile("data.mc",  dataSegment,  0x10000000, 0x50000000);
        dumpSegmentToFile("stack.mc", stackSegment, 0x50000000, 0x7FFFFFFF);

        clockCycle++;

        // Prompt user if not running all
        if (!runAllRemaining) {
            std::cout << "Enter N=next, R=run remainder, E=exit: ";
            std::cin >> userInput;
            if (userInput == 'E' || userInput == 'e') {
                std::cout << "Exiting at user request.\n";
                break;
            } else if (userInput == 'R' || userInput == 'r') {
                runAllRemaining = true;
            }
        }
    }

    std::cout << "Simulation finished after " << clockCycle << " cycles.\n";
    return 0;
}
