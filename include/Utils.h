#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <cstdint>

std::string trim(const std::string &s);
std::vector<std::string> splitTokens(const std::string &line);
int getRegisterNumber(const std::string &reg);
int32_t parseImmediate(const std::string &immStr);
uint32_t setBits(uint32_t value, unsigned offset, unsigned numBits, uint32_t field);
std::string toBinary(uint32_t val, int width);
std::string toHex32(uint32_t val);
std::string toUpper(const std::string &s);

#endif // UTILS_H
