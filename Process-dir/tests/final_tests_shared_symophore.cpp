#include <iostream>
#include <cassert>
#include <string>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/wait.h>
#endif

#include "../include/SharedSemaphore.h"

static const char* SEM_NAME = "/test_sem";

void sleep_seconds(int s) {
#ifdef _WIN32
    Sleep(s * 1000);
#else
    sleep(s);
#endif
}


void run_child_test1() {
    SharedSemaphore s(SEM_NAME, false);
    sleep_seconds(1);
    s.post();
    exit(0);
}

void run_child_test3() {
    SharedSemaphore c(SEM_NAME, false);
    c.wait();
    c.post();
    exit(0);
}


void test_basic_post_wait() {
    std::cout << "TEST 1: basic wait/post\n";

    SharedSemaphore sem(SEM_NAME, true, 0);

#ifdef _WIN32
    char selfPath[MAX_PATH];
    GetModuleFileNameA(nullptr, selfPath, MAX_PATH);
    std::string cmd = "\"" + std::string(selfPath) + "\" child_test1";

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    bool success = CreateProcessA(nullptr, const_cast<char*>(cmd.c_str()), 
                                  nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
    assert(success);
#else
    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
        run_child_test1();
    }
#endif

    sem.wait();

#ifdef _WIN32
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    waitpid(pid, nullptr, 0);
#endif
}

void test_multiple_posts() {
    std::cout << "TEST 2: multiple post\n";

    SharedSemaphore sem(SEM_NAME, true, 0);

    sem.post();
    sem.post();
    sem.post();

    sem.wait();
    sem.wait();
    sem.wait();
}

void test_parent_child_sync() {
    std::cout << "TEST 3: sync\n";

    SharedSemaphore sem(SEM_NAME, true, 0);

#ifdef _WIN32
    char selfPath[MAX_PATH];
    GetModuleFileNameA(nullptr, selfPath, MAX_PATH);
    std::string cmd = "\"" + std::string(selfPath) + "\" child_test3";

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    bool success = CreateProcessA(nullptr, const_cast<char*>(cmd.c_str()), 
                                  nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
    assert(success);
#else
    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
        run_child_test3();
    }
#endif

    sem.post();
    
    sleep_seconds(1);
    
    sem.wait();

#ifdef _WIN32
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    waitpid(pid, nullptr, 0);
#endif
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string mode = argv[1];
        if (mode == "child_test1") {
            run_child_test1();
            return 0;
        }
        if (mode == "child_test3") {
            run_child_test3();
            return 0;
        }
    }

    test_basic_post_wait();
    test_multiple_posts();
    test_parent_child_sync();

    std::cout << "SharedSemaphore tests OK\n";
    return 0;
}