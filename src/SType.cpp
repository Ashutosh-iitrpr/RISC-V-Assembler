#include "SType.h"
#include "Utils.h"
#include <sstream>

uint32_t encodeSType(uint8_t opcode, uint8_t func3,
                     int rs1, int rs2, int32_t imm) {
    uint32_t insn = 0;
    uint32_t imm12 = (uint32_t)(imm & 0xFFF);
    uint32_t immHi = (imm12 >> 5) & 0x7F;
    uint32_t immLo = imm12 & 0x1F;

    insn = setBits(insn, 0, 7, opcode);
    insn = setBits(insn, 7, 5, immLo);
    insn = setBits(insn, 12, 3, func3);
    insn = setBits(insn, 15, 5, rs1);
    insn = setBits(insn, 20, 5, rs2);
    insn = setBits(insn, 25, 7, immHi);
    return insn;
}

std::string buildBitCommentS(uint8_t opcode, uint8_t func3,
                             int rs1, int rs2, int32_t imm12) {
    std::ostringstream oss;
    oss << toBinary(opcode, 7) << "-"
        << toBinary(func3, 3) << "-"
        << "NULL-"
        << toBinary(rs1, 5) << "-"
        << toBinary(rs2, 5) << "-"
        << toBinary(imm12 & 0xFFF, 12);
    return oss.str();
}
