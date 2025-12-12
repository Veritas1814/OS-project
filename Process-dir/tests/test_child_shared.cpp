#include <iostream>
#include "../include/SharedMemoryChannel.h"
#include "../include/SharedSemaphore.h"

int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "Expected shmIn shmOut semIn semOut\n";
        return 1;
    }

    std::string shmInName  = argv[1];
    std::string shmOutName = argv[2];
    std::string semInName  = argv[3];
    std::string semOutName = argv[4];

    SharedMemoryChannel shmIn, shmOut;
    shmIn.open(shmInName, 4096);
    shmOut.open(shmOutName, 4096);

    SharedSemaphore semIn(semInName,  false);
    SharedSemaphore semOut(semOutName, false);

    while (true) {
        semIn.wait();
        std::string msg = shmIn.read();
        if (msg == "exit") break;

        shmOut.write("child: " + msg);
        semOut.post();
    }

    return 0;
}