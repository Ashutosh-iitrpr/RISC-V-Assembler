#ifndef SBTYPE_H
#define SBTYPE_H

#include <cstdint>
#include <string>

uint32_t encodeSBType(uint8_t opcode, uint8_t func3,
                      int rs1, int rs2, int32_t branchOffsetBytes);
std::string buildBitCommentSB(uint8_t opcode, uint8_t func3,
                              int rs1, int rs2, int32_t offset);

#endif // SBTYPE_H
