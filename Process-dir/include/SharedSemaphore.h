#pragma once
#include "SharedMemoryChannel.h"
#include <pthread.h>
#include <string>

class SharedSemaphore {
public:
    SharedSemaphore();
    SharedSemaphore(const std::string& name, bool create, int initialValue = 0);
    ~SharedSemaphore();

    void init(const std::string& name, bool create, int initialValue = 0);
    void wait();
    void post();

private:
    struct SemaphoreData {
        pthread_mutex_t mtx;
        pthread_cond_t  cond;
        int value;
    };

    SharedMemoryChannel shm;
    SemaphoreData* data = nullptr;
    bool creator = false;
};