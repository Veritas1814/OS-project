#include <Process.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    // 1. Define a name (Must start with / on POSIX)
    std::string memName = "/my_test_mem";
    int memSize = 1024;

    if (argc > 1 && std::string(argv[1]) == "child") {
        // --- CHILD PROCESS ---
        std::cout << "[Child] Starting...\n";

        SharedMemoryChannel shm;
        // 2. Open the existing memory
        if (!shm.open(memName, memSize)) {
            std::cerr << "[Child] Failed to open shared memory!\n";
            return 1;
        }

        // 3. Wait slightly to ensure parent wrote (Synchronization is skipped for simplicity here)
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::string msg = shm.read();
        std::cout << "[Child] Read from Memory: " << msg << "\n";
        return 0;
    }

    // --- PARENT PROCESS ---
    std::cout << "[Parent] Creating Shared Memory...\n";

    SharedMemoryChannel shm;
    if (!shm.create(memName, memSize)) {
        std::cerr << "[Parent] Failed to create shared memory!\n";
        return 1;
    }

    // 2. Write data
    shm.write("Hello from Linux Shared Memory!");

    // 3. Launch Child
    Process p(argv[0], {"child"});
    p.start();

    p.wait();
    std::cout << "[Parent] Done.\n";

    return 0;
}