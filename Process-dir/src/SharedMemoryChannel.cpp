// This is a demo version of PVS-Studio for educational use.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include "../include/SharedMemoryChannel.h"
#include <iostream>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

SharedMemoryChannel::SharedMemoryChannel() = default;
SharedMemoryChannel::~SharedMemoryChannel() { close(); }

bool SharedMemoryChannel::create(const std::string& n, size_t sz) {
    name = n;
    size = sz;
    std::cerr << "Creating SHM: " << name << " size=" << size << "\n";

#ifdef _WIN32
    HANDLE h = CreateFileMappingA(
        INVALID_HANDLE_VALUE,    // Use paging file
        nullptr,                 // Default security
        PAGE_READWRITE,          // Read/Write protection
        0,                       // Max size (high 32 bits)
        static_cast<DWORD>(size),// Max size (low 32 bits)
        name.c_str()             // Name of the mapping object
    );

    if (!h) {
        std::cerr << "CreateFileMappingA failed. Error: " << GetLastError() << "\n";
        return false;
    }

    hMap = h; 

    buffer = MapViewOfFile(
        hMap,
        FILE_MAP_ALL_ACCESS, 
        0,
        0,
        size
    );

    if (!buffer) {
        std::cerr << "MapViewOfFile failed. Error: " << GetLastError() << "\n";
        CloseHandle(static_cast<HANDLE>(hMap));
        hMap = nullptr;
        return false;
    }

    return true;
#else
    // POSIX
    fd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd == -1) return false;

    if (ftruncate(fd, size) == -1) return false;

    buffer = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    return buffer != MAP_FAILED;
#endif
}

bool SharedMemoryChannel::open(const std::string& n, size_t sz) {
    name = n;
    size = sz;

#ifdef _WIN32
    HANDLE h = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        name.c_str()
    );

    if (!h) {
        h = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            static_cast<DWORD>(size),
            name.c_str()
        );
    }

    if (!h) {
        std::cerr << "SharedMemoryChannel::open failed (Windows). Error: " << GetLastError() << "\n";
        return false;
    }

    hMap = h;

    buffer = MapViewOfFile(
        hMap,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        size
    );

    if (!buffer) {
        std::cerr << "MapViewOfFile (open) failed. Error: " << GetLastError() << "\n";
        CloseHandle(static_cast<HANDLE>(hMap));
        hMap = nullptr;
        return false;
    }

    return true;
#else
    // POSIX
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
#endif
}

bool SharedMemoryChannel::write(const std::string& data) {
    if (!buffer) return false;

    size_t copySize = (std::min)(size - 1, data.size());
    memcpy(buffer, data.data(), copySize);

    ((char*)buffer)[copySize] = '\0';

    return true;
}

std::string SharedMemoryChannel::read() {
    if (!buffer) return "";
    return std::string((char*)buffer);
}

void SharedMemoryChannel::close() {
#ifdef _WIN32
    if (buffer) {
        UnmapViewOfFile(buffer);
        buffer = nullptr;
    }
    if (hMap) {
        CloseHandle(static_cast<HANDLE>(hMap));
        hMap = nullptr;
    }
#else
    if (buffer) {
        munmap(buffer, size);
        buffer = nullptr;
    }
    if (fd != -1) {
        shm_unlink(name.c_str());
        fd = -1;
    }
#endif
}