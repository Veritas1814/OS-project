#include <Process.h>
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    std::string semName = "/my_test_sem";

    if (argc > 1 && std::string(argv[1]) == "child") {
        // --- CHILD PROCESS ---
        std::cout << "[Child] Attempting to acquire semaphore...\n";

        // 1. Open existing semaphore
        SharedSemaphore sem(semName, false, 0);

        // 2. WAIT (Blocks until count > 0)
        sem.wait();

        std::cout << "[Child] Acquired! I can now proceed safely.\n";
        return 0;
    }

    // --- PARENT PROCESS ---
    // 1. Create semaphore, init value 0 (Locked)
    SharedSemaphore sem(semName, true, 0);

    Process p(argv[0], {"child"});
    p.start();

    std::cout << "[Parent] Child launched. I will hold the lock for 2 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "[Parent] Releasing semaphore now!\n";
    // 2. RELEASE
    sem.post();

    p.wait();
    return 0;
}