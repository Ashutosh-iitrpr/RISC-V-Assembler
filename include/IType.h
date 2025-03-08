#ifndef ITYPE_H
#define ITYPE_H

#include <cstdint>
#include <string>

uint32_t encodeIType(uint8_t opcode, uint8_t func3,
                     int rd, int rs1, int32_t imm12);
std::string buildBitCommentI(uint8_t opcode, uint8_t func3,
                             int rd, int rs1, int32_t imm12);

#endif // ITYPE_H
