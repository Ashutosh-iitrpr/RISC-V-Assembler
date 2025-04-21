// main.cpp

#include <iostream>
#include <cstdlib>

// Forward‑declare the two entry points, in their namespaces:
namespace pipelined {
    int simulate(int argc, char** argv);
}
namespace unpipelined {
    int simulate(int argc, char** argv);
}

int main(int argc, char** argv) {
    // We expect exactly 5 user args + program name: 
    //   argv[1] = input.mc
    //   argv[2] = data.mc
    //   argv[3] = stack.mc
    //   argv[4] = instruction.mc
    if (argc != 5) {
        std::cerr 
            << "Usage: " << argv[0]
            << " <mem.mc> <data.mc> <stack.mc> <instr.mc>\n";
        return 1;
    }
     const int knob1 = 1; 

    if (knob1 != 0 && knob1 != 1) {
        std::cerr << "Error: knob1 must be 0 or 1\n";
        return 1;
    }

    // Dispatch to the chosen simulator:
    if (knob1) {
        // Call pipelined::simulate with all argv[]
        return pipelined::simulate(argc, argv);
    } else {
        return unpipelined::simulate(argc, argv);
    }
}

