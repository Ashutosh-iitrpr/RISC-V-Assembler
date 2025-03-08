#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>

static const uint32_t TEXT_START = 0x00000000;      // Starting address for .text
static const uint32_t DATA_START = 0x10000000;        // Starting address for .data
static const uint32_t CODE_STEP  = 4;                 // Each instruction is 4 bytes

enum Section {
    UNDEF,
    TEXT,
    DATA
};

#endif // CONSTANTS_H
