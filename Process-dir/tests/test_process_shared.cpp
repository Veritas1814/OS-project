#include <iostream>
#include <cassert>
#include <string>
#include "../include/Process.h"

void test_basic_exchange() {
    std::cout << "\n===== TEST 1: Parent <-> Child basic exchange =====\n";

#ifdef _WIN32
    std::string exe = "test_child_shared.exe";
#else
    std::string exe = "./test_child_shared";
#endif

    Process p(exe,
              {"/proc_shm_in_test",
               "/proc_shm_out_test",
               "/proc_sem_in_test",
               "/proc_sem_out_test"});

    bool ok = p.startSharedMemory();
    assert(ok);

    p.writeStdin("hello");
    std::string out = p.readStdout();
    std::cout << "[parent] got: " << out << "\n";

    assert(out == "child: hello");

    p.writeStdin("world");
    out = p.readStdout();
    std::cout << "[parent] got: " << out << "\n";

    assert(out == "child: world");

    p.writeStdin("exit");

    p.wait();
    std::cout << "Test 1 passed.\n";
}

void test_multiple_rounds() {
    std::cout << "\n===== TEST 2: Multiple message rounds =====\n";

#ifdef _WIN32
    std::string exe = "test_child_shared.exe";
#else
    std::string exe = "./test_child_shared";
#endif

    Process p(exe,
              {"/proc_shm_in_multi",
               "/proc_shm_out_multi",
               "/proc_sem_in_multi",
               "/proc_sem_out_multi"});

    assert(p.startSharedMemory());

    for (int i = 0; i < 5; i++) {
        std::string msg = "msg_" + std::to_string(i);
        p.writeStdin(msg);

        std::string out = p.readStdout();
        std::cout << "exchange " << i << ": " << out << "\n";

        assert(out == "child: " + msg);
    }

    p.writeStdin("exit");
    p.wait();

    std::cout << "Test 2 passed.\n";
}

void test_large_message() {
    std::cout << "\n===== TEST 3: Large message =====\n";

#ifdef _WIN32
    std::string exe = "test_child_shared.exe";
#else
    std::string exe = "./test_child_shared";
#endif

    Process p(exe,
              {"/proc_shm_in_large",
               "/proc_shm_out_large",
               "/proc_sem_in_large",
               "/proc_sem_out_large"});

    assert(p.startSharedMemory(8192));

    std::string big(4000, 'A');
    p.writeStdin(big);

    std::string out = p.readStdout();

    assert(out == "child: " + big);

    std::cout << "Test 3 passed.\n";

    p.writeStdin("exit");
    p.wait();
}

int main() {
    test_basic_exchange();
    test_multiple_rounds();
    test_large_message();

    std::cout << "\nAll shared memory IPC tests passed.\n";
    return 0;
}