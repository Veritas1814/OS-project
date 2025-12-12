// This is a demo version of PVS-Studio for educational use.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#pragma once
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include "SharedMemoryChannel.h"
#include <pthread.h>
#endif

class SharedSemaphore {
public:
    SharedSemaphore();
    SharedSemaphore(const std::string& name, bool create, int initialValue = 0);
    ~SharedSemaphore();

    void init(const std::string& name, bool create, int initialValue = 0);
    void wait();
    void post();

private:
#ifdef _WIN32
    HANDLE hSem = NULL;
    bool creator = false;
#else
    struct SemaphoreData {
        pthread_mutex_t mtx;
        pthread_cond_t  cond;
        int value;
    };

    SharedMemoryChannel shm;
    SemaphoreData* data = nullptr;
    bool creator = false;
#endif
};