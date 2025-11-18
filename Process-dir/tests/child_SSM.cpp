#include "../include/Process.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "ERROR: Shared memory names not provided.\n";
        std::cerr << "argc = " << argc << "\n";
        for (int i = 0; i < argc; i++)
            std::cerr << "argv[" << i << "] = " << argv[i] << "\n";
        return 1;
    }

    const size_t SIZE = 4096;

    SharedMemoryChannel in, out, err;

    if (!in.open(argv[1], SIZE)) {
        std::cerr << "ERROR: cannot open SHM IN: " << argv[1] << "\n";
        return 1;
    }

    if (!out.open(argv[2], SIZE)) {
        std::cerr << "ERROR: cannot open SHM OUT: " << argv[2] << "\n";
        return 1;
    }

    if (!err.open(argv[3], SIZE)) {
        std::cerr << "ERROR: cannot open SHM ERR: " << argv[3] << "\n";
        return 1;
    }

    std::string msg = in.read();

    out.write("child got: " + msg);
    err.write("child finished OK");

    return 0;
}
