#include <cassert>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include "../include/SharedSemaphore.h"

static const char* SEM_NAME = "/test_sem";

void test_basic_post_wait() {
    std::cout << "TEST 1: basic wait/post\n";

    SharedSemaphore sem(SEM_NAME, true, 0);

    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
        SharedSemaphore s(SEM_NAME, false);
        sleep(1);
        s.post();
        _exit(0);
    }

    sem.wait();
    waitpid(pid, nullptr, 0);
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

    pid_t pid = fork();

    if (pid == 0) {
        SharedSemaphore c(SEM_NAME, false);
        c.wait();
        c.post();
        _exit(0);
    }

    sem.post();
    sleep(1);
    sem.wait();

    waitpid(pid, nullptr, 0);
}

int main() {
    test_basic_post_wait();
    test_multiple_posts();
    test_parent_child_sync();

    std::cout << "SharedSemaphore tests OK\n";
}