#include <cassert>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#include "../include/SharedMemoryChannel.h"

static const char* SHM_NAME = "/test_shm_channel";
static const size_t SHM_SIZE = 4096;

void test_create_and_write_read() {
    std::cout << "TEST 1: basic create/write/read\n";

    SharedMemoryChannel shm;
    assert(shm.create(SHM_NAME, SHM_SIZE));

    shm.write("hello");
    std::string out = shm.read();
    assert(out == "hello");
}

void test_open_existing_segment() {
    std::cout << "TEST 2: open existing\n";

    SharedMemoryChannel shm;
    assert(shm.open(SHM_NAME, SHM_SIZE));

    shm.write("test123");
    std::string out = shm.read();
    assert(out == "test123");
}

void test_large_write() {
    std::cout << "TEST 3: large write\n";

    SharedMemoryChannel shm;
    shm.open(SHM_NAME, SHM_SIZE);

    std::string big(3000, 'A');
    shm.write(big);

    std::string out = shm.read();
    assert(out == big);
}

void test_parent_child_ipc() {
    std::cout << "TEST 4: parent <-> child\n";

    SharedMemoryChannel shm;
    assert(shm.create("/ipc_mem", SHM_SIZE));

    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
        SharedMemoryChannel c;
        c.open("/ipc_mem", SHM_SIZE);
        std::string msg = c.read();

        c.write("child_answer:" + msg);
        _exit(0);
    }

    shm.write("from_parent");
    waitpid(pid, nullptr, 0);

    std::string out = shm.read();
    assert(out == "child_answer:from_parent");
}

int main() {
    test_create_and_write_read();
    test_open_existing_segment();
    test_large_write();
    test_parent_child_ipc();
    std::cout << "SharedMemoryChannel tests OK\n";
}