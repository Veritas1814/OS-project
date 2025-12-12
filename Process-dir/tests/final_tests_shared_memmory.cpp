// This is a demo version of PVS-Studio for educational use.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include <iostream>
#include <string>
#include <vector>
#include <cassert>

#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
#else
    #include <sys/wait.h>
    #include <unistd.h>
#endif

#include "../include/SharedMemoryChannel.h"

static const char* SHM_NAME = "/test_shm_channel";
static const size_t SHM_SIZE = 4096;

void run_child_logic() {
    SharedMemoryChannel c;
    if (!c.open("/ipc_mem", SHM_SIZE)) {
        std::cerr << "[child] Failed to open shared memory\n";
        exit(1);
    }

    std::string msg = c.read();
    
    c.write("child_answer:" + msg);
    
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "child_worker") {
        run_child_logic();
        return 0;
    }

    std::cout << "SharedMemoryChannel Tests:\n";

    {
        std::cout << "Test 1: basic create/write/read\n";
        SharedMemoryChannel shm;
        if (!shm.create(SHM_NAME, SHM_SIZE)) {
            std::cerr << "Failed to create shared memory\n";
            return 1;
        }

        shm.write("hello");
        std::string out = shm.read();
        
        if (out == "hello") std::cout << "[PASSED]\n\n";
        else std::cout << "[FAILED] Got: " << out << "\n\n";
    }

    {
        std::cout << "Test 2: open existing segment\n";
        SharedMemoryChannel shm;
        if (!shm.open(SHM_NAME, SHM_SIZE)) {
             std::cerr << "Failed to open shared memory\n";
             return 1;
        }

        shm.write("test123");
        std::string out = shm.read();

        if (out == "test123") std::cout << "[PASSED]\n\n";
        else std::cout << "[FAILED] Got: " << out << "\n\n";
    }

    {
        std::cout << "Test 3: large write\n";
        SharedMemoryChannel shm;
        shm.open(SHM_NAME, SHM_SIZE);

        std::string big(3000, 'A');
        shm.write(big);

        std::string out = shm.read();
        
        if (out == big) std::cout << "[PASSED]\n\n";
        else std::cout << "[FAILED] Size mismatch\n\n";
    }

    {
        std::cout << "Test 4: parent <-> child IPC\n";
        SharedMemoryChannel shm;
        if (!shm.create("/ipc_mem", SHM_SIZE)) {
            std::cerr << "Failed to create IPC memory\n";
            return 1;
        }

        shm.write("from_parent");

#ifdef _WIN32
        char selfPath[MAX_PATH];
        GetModuleFileNameA(nullptr, selfPath, MAX_PATH);

        std::string cmd = "\"" + std::string(selfPath) + "\" child_worker";
        
        STARTUPINFOA si{};
        PROCESS_INFORMATION pi{};
        si.cb = sizeof(si);

        if (!CreateProcessA(nullptr, const_cast<char*>(cmd.c_str()), 
                           nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            std::cerr << "CreateProcess failed\n";
            return 1;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

#else
        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "Fork failed\n";
            return 1;
        }

        if (pid == 0) {
            run_child_logic(); 
        }
        waitpid(pid, nullptr, 0);
#endif

        std::string out = shm.read();
        if (out == "child_answer:from_parent") {
            std::cout << "[PASSED]\n\n";
        } else {
            std::cout << "[FAILED] Got: " << out << "\n\n";
        }
    }

    std::cout << "All tests done.\n";
    return 0;
}