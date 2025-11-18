#include "../include/Process.h"
#include <iostream>

int main() {
    Process p("./child_shm", {});
    p.startSharedMemory("/my_shm_test", 4096);

    p.writeStdin("hello via shared memory");
    p.closeStdin();

    p.wait();

    std::cout << "OUT: " << p.readStdout() << "\n";
    std::cout << "ERR: " << p.readStderr() << "\n";

    return 0;
}
