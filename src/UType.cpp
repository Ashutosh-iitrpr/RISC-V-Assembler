#include "UType.h"
#include "Utils.h"
#include <sstream>

uint32_t encodeUType(uint8_t opcode, int rd, int32_t imm) {
    uint32_t val20 = ((uint32_t)imm) & 0xFFFFF;
    uint32_t insn = 0;
    insn = setBits(insn, 0, 7, opcode);
    insn = setBits(insn, 7, 5, rd);
    insn = setBits(insn, 12, 20, val20);
    return insn;
}

std::string buildBitCommentU(uint8_t opcode, int rd, int32_t imm) {
    uint32_t val20 = ((uint32_t)imm) & 0xFFFFF;
    std::ostringstream oss;
    oss << toBinary(opcode, 7) << "-"
        << "NULL-NULL-"
        << toBinary(rd, 5) << "-"
        << toBinary(val20, 20);
    return oss.str();
}
