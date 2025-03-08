#include "UJType.h"
#include "Utils.h"
#include <sstream>

uint32_t encodeUJType(uint8_t opcode, int rd, int32_t offsetBytes) {
    int32_t offset = offsetBytes >> 1;
    uint32_t imm = (uint32_t)offset & 0xFFFFF;

    uint32_t imm_20   = (imm >> 19) & 0x1;
    uint32_t imm_10_1 = (imm >>  1) & 0x3FF;
    uint32_t imm_11   = (imm >>  0) & 0x1;
    uint32_t imm_19_12= (imm >> 11) & 0xFF;

    uint32_t insn = 0;
    insn = setBits(insn, 0, 7, opcode);
    insn = setBits(insn, 7, 5, rd);
    insn = setBits(insn, 12, 8, imm_19_12);
    insn = setBits(insn, 20, 1, imm_11);
    insn = setBits(insn, 21, 10, imm_10_1);
    insn = setBits(insn, 31, 1, imm_20);
    return insn;
}

std::string buildBitCommentUJ(uint8_t opcode, int rd, int32_t offsetBytes) {
    uint32_t val20 = ((uint32_t)(offsetBytes >> 1)) & 0xFFFFF;
    std::ostringstream oss;
    oss << toBinary(opcode, 7) << "-"
        << "NULL-NULL-"
        << toBinary(rd, 5) << "-"
        << toBinary(val20, 20);
    return oss.str();
}
