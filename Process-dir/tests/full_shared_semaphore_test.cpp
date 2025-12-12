#include "../include/SharedSemaphore.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    #define SLEEP_MS(x) Sleep(x)
#else
    #include <unistd.h>
    #include <sys/wait.h>
    #define SLEEP_MS(x) usleep((x)*1000)
#endif

using std::cout;
using std::endl;

void spawn_child(const std::string& mode) {
#ifdef _WIN32
    char selfPath[MAX_PATH];
    GetModuleFileNameA(nullptr, selfPath, MAX_PATH);
    std::string cmd = "\"" + std::string(selfPath) + "\" " + mode;

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    if (CreateProcessA(nullptr, const_cast<char*>(cmd.c_str()), 
                       nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess); 
    } else {
        std::cerr << "CreateProcess failed for mode: " << mode << "\n";
    }
#else
    pid_t pid = fork();
    if (pid == 0) {
        execl("/proc/self/exe", "self", mode.c_str(), nullptr);
        exit(0);
    }
#endif
}

void wait_for_children(int count) {
#ifdef _WIN32
    SLEEP_MS(500 * count);
#else
    for (int i = 0; i < count; i++) wait(nullptr);
#endif
}

void child_test1_logic() {
    SharedSemaphore sem("/sem_test1", false);
    cout << "[child] waiting...\n";
    sem.wait();
    cout << "[child] unblocked!\n";
    exit(0);
}

void child_test2_logic() {
    SharedSemaphore sem("/sem_test2", false);
    cout << "[child] waiting...\n";
    sem.wait();
    cout << "[child] passed!\n";
    exit(0);
}

void child_test3_logic_1() {
    SharedSemaphore sem("/sem_test3", false); 
    for (int i = 0; i < 3; i++) {
        sem.wait();
        cout << "[child1] CS " << i << endl;
        SLEEP_MS(200);
        sem.post();
    }
    exit(0);
}

void child_test3_logic_2() {
    SharedSemaphore sem("/sem_test3", false);
    for (int i = 0; i < 3; i++) {
        sem.wait();
        cout << "[child2] CS " << i << endl;
        SLEEP_MS(200);
        sem.post();
    }
    exit(0);
}

void child_test4_logic() {
    SharedSemaphore sem("/sem_test4", false);
    cout << "[child] waiting 2 times...\n";
    sem.wait();
    sem.wait();
    cout << "[child] passed both waits!\n";
    exit(0);
}

void child_test5_logic() {
    SharedSemaphore sem2("/sem_test5", false);
    cout << "[child] waiting...\n";
    sem2.wait();
    cout << "[child] OK\n";
    exit(0);
}

void test_basic_wait_post() {
    cout << "\n===== TEST 1: basic wait/post =====\n";
    SharedSemaphore sem("/sem_test1", true, 0);

    spawn_child("child_test1");

    SLEEP_MS(1000);
    cout << "[parent] posting\n";
    sem.post();

    wait_for_children(1);
}

void test_multi_process_sync() {
    cout << "\n===== TEST 2: multi-process sync =====\n";
    const int N = 3;
    SharedSemaphore sem("/sem_test2", true, 0);

    for (int i = 0; i < N; i++) {
        spawn_child("child_test2");
    }

    SLEEP_MS(1000);
    cout << "[parent] posting " << N << " times...\n";
    for (int i = 0; i < N; i++) sem.post();

    wait_for_children(N);
}

void test_mutex_behavior() {
    cout << "\n===== TEST 3: semaphore as mutex =====\n";
    SharedSemaphore sem("/sem_test3", true, 1); 

    spawn_child("child_test3_1");
    spawn_child("child_test3_2");

    wait_for_children(2);
    cout << "If no two CS lines were interleaved -> semaphore works as mutex\n";
}

void test_counter_correctness() {
    cout << "\n===== TEST 4: counter correctness =====\n";
    SharedSemaphore sem("/sem_test4", true, 0); 

    spawn_child("child_test4");

    SLEEP_MS(1000);
    cout << "[parent] posting once\n";
    sem.post();
    SLEEP_MS(1000);

    cout << "[parent] posting second time\n";
    sem.post();

    wait_for_children(1);
}

void test_reopen_shared_memory() {
    cout << "\n===== TEST 5: re-opening existing shared memory =====\n";
    SharedSemaphore sem1("/sem_test5", true, 0);

    spawn_child("child_test5");

    SLEEP_MS(1000);
    cout << "[parent] post\n";
    sem1.post();

    wait_for_children(1);
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string mode = argv[1];
        if (mode == "child_test1") child_test1_logic();
        if (mode == "child_test2") child_test2_logic();
        if (mode == "child_test3_1") child_test3_logic_1();
        if (mode == "child_test3_2") child_test3_logic_2();
        if (mode == "child_test4") child_test4_logic();
        if (mode == "child_test5") child_test5_logic();
        return 0;
    }

    cout << "\n=======================================\n";
    cout << "     SharedSemaphore FULL TEST SUITE    \n";
    cout << "=======================================\n";

    test_basic_wait_post();
    test_multi_process_sync();
    test_mutex_behavior();
    test_counter_correctness();
    test_reopen_shared_memory();

    cout << "\nAll tests completed successfully.\n";
    return 0;
}