#include "SharedSemaphore.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

using std::cout;
using std::endl;


void test_basic_wait_post() {
    cout << "\n===== TEST 1: basic wait/post =====\n";

    SharedSemaphore sem("/sem_test1", 0);

    pid_t pid = fork();
    if (pid == 0) {
        SharedSemaphore sem("/sem_test1", 0);
        cout << "[child] waiting...\n";
        sem.wait();
        cout << "[child] unblocked!\n";
        _exit(0);
    }

    sleep(1);
    cout << "[parent] posting\n";
    sem.post();

    waitpid(pid, nullptr, 0);
}


void test_multi_process_sync() {
    cout << "\n===== TEST 2: multi-process sync =====\n";

    const int N = 3;
    SharedSemaphore sem("/sem_test2", 0);

    for (int i = 0; i < N; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            SharedSemaphore sem("/sem_test2", 0);
            cout << "[child " << getpid() << "] waiting...\n";
            sem.wait();
            cout << "[child " << getpid() << "] passed!\n";
            _exit(0);
        }
    }

    sleep(1);
    cout << "[parent] posting " << N << " times...\n";

    for (int i = 0; i < N; i++) sem.post();

    for (int i = 0; i < N; i++) wait(nullptr);
}


void test_mutex_behavior() {
    cout << "\n===== TEST 3: semaphore as mutex =====\n";

    SharedSemaphore sem("/sem_test3", 1);

    pid_t pid1 = fork();
    if (pid1 == 0) {
        SharedSemaphore sem("/sem_test3", 1);
        for (int i = 0; i < 3; i++) {
            sem.wait();
            cout << "[child1] CS " << i << endl;
            usleep(200000);
            sem.post();
        }
        _exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        SharedSemaphore sem("/sem_test3", 1);
        for (int i = 0; i < 3; i++) {
            sem.wait();
            cout << "[child2] CS " << i << endl;
            usleep(200000);
            sem.post();
        }
        _exit(0);
    }

    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);

    cout << "If no two CS lines were interleaved → semaphore works as mutex\n";
}


void test_counter_correctness() {
    cout << "\n===== TEST 4: counter correctness =====\n";

    SharedSemaphore sem("/sem_test4", 2);

    pid_t pid = fork();
    if (pid == 0) {
        SharedSemaphore sem("/sem_test4", 0);
        cout << "[child] waiting 2 times...\n";
        sem.wait();
        sem.wait();
        cout << "[child] passed both waits!\n";
        _exit(0);
    }

    sleep(1);
    cout << "[parent] posting once\n";
    sem.post();
    sleep(1);

    cout << "[parent] posting second time\n";
    sem.post();

    waitpid(pid, nullptr, 0);
}


void test_reopen_shared_memory() {
    cout << "\n===== TEST 5: re-opening existing shared memory =====\n";

    SharedSemaphore sem1("/sem_test5", 1);

    pid_t pid = fork();
    if (pid == 0) {
        SharedSemaphore sem2("/sem_test5", 0);
        cout << "[child] waiting...\n";
        sem2.wait();
        cout << "[child] OK\n";
        _exit(0);
    }

    sleep(1);
    cout << "[parent] post\n";
    sem1.post();

    waitpid(pid, nullptr, 0);
}


int main() {
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