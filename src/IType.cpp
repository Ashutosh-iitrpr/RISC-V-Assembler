#include "IType.h"
#include "Utils.h"
#include <sstream>

uint32_t encodeIType(uint8_t opcode, uint8_t func3,
                     int rd, int rs1, int32_t imm12) {
    uint32_t insn = 0;
    uint32_t imm = ((uint32_t)imm12 & 0xFFF);
    insn = setBits(insn, 0, 7, opcode);
    insn = setBits(insn, 7, 5, rd);
    insn = setBits(insn, 12, 3, func3);
    insn = setBits(insn, 15, 5, rs1);
    insn = setBits(insn, 20, 12, imm);
    return insn;
}

std::string buildBitCommentI(uint8_t opcode, uint8_t func3,
                             int rd, int rs1, int32_t imm12) {
    std::ostringstream oss;
    oss << toBinary(opcode, 7) << "-"
        << toBinary(func3, 3) << "-"
        << "NULL-"
        << toBinary(rd, 5) << "-"
        << toBinary(rs1, 5) << "-"
        << toBinary(imm12 & 0xFFF, 12);
    return oss.str();
}
