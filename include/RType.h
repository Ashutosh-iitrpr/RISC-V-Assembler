#ifndef RTYPE_H
#define RTYPE_H

#include <cstdint>
#include <string>

uint32_t encodeRType(uint8_t opcode, uint8_t func3, uint8_t func7,
                     int rd, int rs1, int rs2);
std::string buildBitCommentR(uint8_t opcode, uint8_t func3, uint8_t func7,
                             int rd, int rs1, int rs2);

#endif // RTYPE_H
