#ifndef UJTYPE_H
#define UJTYPE_H

#include <cstdint>
#include <string>

uint32_t encodeUJType(uint8_t opcode, int rd, int32_t offsetBytes);
std::string buildBitCommentUJ(uint8_t opcode, int rd, int32_t offsetBytes);

#endif // UJTYPE_H
