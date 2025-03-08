#include "assembler.h"

int main() {
    // Change these filenames as needed.
    Assembler assembler("input.asm", "output.mc");
    assembler.assemble();
    return 0;
}
