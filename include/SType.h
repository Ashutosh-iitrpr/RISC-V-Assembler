#ifndef STYPE_H
#define STYPE_H

#include <cstdint>
#include <string>

uint32_t encodeSType(uint8_t opcode, uint8_t func3,
                     int rs1, int rs2, int32_t imm);
std::string buildBitCommentS(uint8_t opcode, uint8_t func3,
                             int rs1, int rs2, int32_t imm12);

#endif // STYPE_H
