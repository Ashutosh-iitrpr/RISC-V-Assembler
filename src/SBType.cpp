#include "SBType.h"
#include "Utils.h"
#include <sstream>

uint32_t encodeSBType(uint8_t opcode, uint8_t func3,
                      int rs1, int rs2, int32_t branchOffsetBytes) {
    int32_t offset = branchOffsetBytes >> 0;
    uint32_t imm12 = (uint32_t)offset & 0xFFF;

    uint32_t insn = 0;
    uint32_t imm_4_1 = (imm12 >> 1) & 0xF;
    uint32_t imm_11  = (imm12 >> 11) & 0x1;
    uint32_t imm_10_5 = (imm12 >> 5) & 0x3F;
    uint32_t imm_12 = (imm12 >> 12) & 0x1;

    insn = setBits(insn, 0, 7, opcode);
    insn = setBits(insn, 8, 4, imm_4_1);
    insn = setBits(insn, 7, 1, imm_11);
    insn = setBits(insn, 25, 6, imm_10_5);
    insn = setBits(insn, 31, 1, imm_12);
    insn = setBits(insn, 12, 3, func3);
    insn = setBits(insn, 15, 5, rs1);
    insn = setBits(insn, 20, 5, rs2);
    return insn;
}

std::string buildBitCommentSB(uint8_t opcode, uint8_t func3,
                              int rs1, int rs2, int32_t offset) {
    uint32_t imm12 = (offset >> 1) & 0xFFF;
    std::ostringstream oss;
    oss << toBinary(opcode, 7) << "-"
        << toBinary(func3, 3) << "-"
        << "NULL-"
        << toBinary(rs1, 5) << "-"
        << toBinary(rs2, 5) << "-"
        << toBinary(imm12, 12);
    return oss.str();
}
