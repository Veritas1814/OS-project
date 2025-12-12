#include "../include/SharedMemoryChannel.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <unistd.h>

SharedMemoryChannel::SharedMemoryChannel() = default;
SharedMemoryChannel::~SharedMemoryChannel() { close(); }


bool SharedMemoryChannel::create(const std::string& n, size_t sz) {
    name = n;
    size = sz;
    std::cerr << "Creating SHM: " << name << " size=" << size << "\n";
    fd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd == -1) return false;

    if (ftruncate(fd, size) == -1) return false;

    buffer = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    return buffer != MAP_FAILED;
}

bool SharedMemoryChannel::open(const std::string& n, size_t sz) {
    name = n;
    size = sz;

    fd = shm_open(name.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("shm_open (open) failed");
        return false;
    }

    struct stat st;
    fstat(fd, &st);
    if (st.st_size < (off_t)size) {
        if (ftruncate(fd, size) == -1) {
            perror("ftruncate failed");
            return false;
        }
    }

    buffer = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buffer == MAP_FAILED) {
        perror("mmap failed");
        return false;
    }

    return true;
}

bool SharedMemoryChannel::write(const std::string& data) {
    if (!buffer) return false;

    size_t copySize = std::min(size - 1, data.size());
    memcpy(buffer, data.data(), copySize);

    ((char*)buffer)[copySize] = '\0';

    return true;
}

std::string SharedMemoryChannel::read() {
    if (!buffer) return "";
    return std::string((char*)buffer);
}

void SharedMemoryChannel::close() {
    if (buffer) {
        munmap(buffer, size);
        buffer = nullptr;
    }
    if (fd != -1) {
        shm_unlink(name.c_str());
        fd = -1;
    }
}


