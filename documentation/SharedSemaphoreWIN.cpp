#include <Process.h>
#include <iostream>
#include <windows.h>

int main(int argc, char* argv[]) {
    std::string semName = "Local\\MyTestSem";

    if (argc > 1 && std::string(argv[1]) == "child") {
        // --- CHILD PROCESS ---
        std::cout << "[Child] Attempting to acquire semaphore...\n";

        // 1. Open the semaphore (create=false)
        SharedSemaphore sem(semName, false, 0);

        // 2. WAIT: This will block until the parent calls post()
        sem.wait();

        std::cout << "[Child] Acquired! I can now proceed safely.\n";
        return 0;
    }

    // --- PARENT PROCESS ---
    // 1. Create semaphore with count 0 (locked initially)
    SharedSemaphore sem(semName, true, 0);

    Process p(argv[0], {"child"});
    p.start();

    std::cout << "[Parent] Child launched. I will hold the lock for 2 seconds...\n";
    Sleep(2000); // Simulate some work

    std::cout << "[Parent] Releasing semaphore now!\n";
    // 2. RELEASE: Increments count, allowing child to wake up
    sem.post();

    p.wait();
    return 0;
}