#include "RType.h"
#include "Utils.h"
#include <sstream>

uint32_t encodeRType(uint8_t opcode, uint8_t func3, uint8_t func7,
                     int rd, int rs1, int rs2) {
    uint32_t insn = 0;
    insn = setBits(insn, 0, 7, opcode);
    insn = setBits(insn, 7, 5, rd);
    insn = setBits(insn, 12, 3, func3);
    insn = setBits(insn, 15, 5, rs1);
    insn = setBits(insn, 20, 5, rs2);
    insn = setBits(insn, 25, 7, func7);
    return insn;
}

std::string buildBitCommentR(uint8_t opcode, uint8_t func3, uint8_t func7,
                             int rd, int rs1, int rs2) {
    std::ostringstream oss;
    oss << toBinary(opcode, 7) << "-"
        << toBinary(func3, 3) << "-"
        << toBinary(func7, 7) << "-"
        << toBinary(rd, 5) << "-"
        << toBinary(rs1, 5) << "-"
        << toBinary(rs2, 5) << "-"
        << "NULL";
    return oss.str();
}
