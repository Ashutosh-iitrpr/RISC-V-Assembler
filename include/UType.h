#ifndef UTYPE_H
#define UTYPE_H

#include <cstdint>
#include <string>

uint32_t encodeUType(uint8_t opcode, int rd, int32_t imm);
std::string buildBitCommentU(uint8_t opcode, int rd, int32_t imm);

#endif // UTYPE_H
