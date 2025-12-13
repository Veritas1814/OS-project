#include <Process.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // 1. Define a name for our shared memory block
    std::string memName = "Local\\MyTestMem";
    int memSize = 1024;

    if (argc > 1 && std::string(argv[1]) == "child") {
        // --- CHILD PROCESS ---
        std::cout << "[Child] Starting...\n";

        // 2. Open the existing shared memory created by the parent
        SharedMemoryChannel shm;
        if (!shm.open(memName, memSize)) {
            std::cerr << "[Child] Failed to open shared memory!\n";
            return 1;
        }

        // 3. Read the data directly
        // Note: In real code, you would use a semaphore to know WHEN to read.
        // Here we just sleep briefly to ensure parent has written.
        Sleep(1000);
        std::string msg = shm.read();

        std::cout << "[Child] Read from Memory: " << msg << "\n";
        return 0;
    }

    // --- PARENT PROCESS ---
    std::cout << "[Parent] Creating Shared Memory...\n";

    // 1. Create the shared memory
    SharedMemoryChannel shm;
    if (!shm.create(memName, memSize)) {
        std::cerr << "[Parent] Failed to create shared memory!\n";
        return 1;
    }

    // 2. Write data into it
    shm.write("Hello from Windows Shared Memory!");

    // 3. Launch the child process
    // We pass "child" as an argument so the executable knows which mode to run
    Process p(argv[0], {"child"});
    p.start();

    // 4. Wait for child to finish reading
    p.wait();
    std::cout << "[Parent] Child finished.\n";

    return 0;
}