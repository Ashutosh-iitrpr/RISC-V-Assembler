#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <cctype>
#include <iostream>

std::string trim(const std::string &s) {
    auto start = s.find_first_not_of(" \t\r\n");
    auto end   = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

std::vector<std::string> splitTokens(const std::string &line) {
    std::vector<std::string> tokens;
    std::string temp;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (isspace((unsigned char)c) || c == ',') {
            if (!temp.empty()) {
                tokens.push_back(temp);
                temp.clear();
            }
        } else {
            temp.push_back(c);
        }
    }
    if (!temp.empty())
        tokens.push_back(temp);
    return tokens;
}

int getRegisterNumber(const std::string &reg) {
    if (reg.size() < 2 || reg[0] != 'x') {
        std::cerr << "[ERROR] Invalid register name: " << reg << std::endl;
        return 0;
    }
    int num = std::stoi(reg.substr(1));
    if (num < 0 || num > 31) {
        std::cerr << "[ERROR] Invalid register number: " << reg << std::endl;
        return 0;
    }
    return num;
}

int32_t parseImmediate(const std::string &immStr) {
    if (immStr.size() > 2 && immStr[0] == '0' &&
       (immStr[1] == 'x' || immStr[1] == 'X')) {
        return static_cast<int32_t>(strtol(immStr.c_str(), nullptr, 16));
    } else {
        return std::stoi(immStr);
    }
}

uint32_t setBits(uint32_t value, unsigned offset, unsigned numBits, uint32_t field) {
    uint32_t mask = ((1u << numBits) - 1u) << offset;
    value &= ~mask;
    value |= ((field & ((1u << numBits) - 1u)) << offset);
    return value;
}

std::string toBinary(uint32_t val, int width) {
    std::string s(width, '0');
    for (int i = 0; i < width; i++) {
        int bit = (val >> i) & 1;
        s[width - 1 - i] = char('0' + bit);
    }
    return s;
}

std::string toHex32(uint32_t val) {
    std::ostringstream oss;
    oss << "0x" << std::setw(8) << std::setfill('0')
        << std::hex << std::uppercase << val;
    return oss.str();
}

std::string toUpper(const std::string &s) {
    std::string res = s;
    for (auto &c : res) c = toupper(c);
    return res;
}
