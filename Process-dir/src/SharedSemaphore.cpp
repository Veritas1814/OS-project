// This is a demo version of PVS-Studio for educational use.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include "../include/SharedSemaphore.h"
#include <stdexcept>
#include <string>
#include <iostream>

#ifdef _WIN32
SharedSemaphore::SharedSemaphore() : hSem(NULL), creator(false) {}

SharedSemaphore::SharedSemaphore(const std::string& name, bool create, int initialValue) 
    : hSem(NULL), creator(false) 
{
    init(name, create, initialValue);
}

SharedSemaphore::~SharedSemaphore() {
    if (hSem) {
        CloseHandle(hSem);
        hSem = NULL;
    }
}

void SharedSemaphore::init(const std::string& name, bool create, int initialValue) {
    if (hSem) {
        CloseHandle(hSem);
        hSem = NULL;
    }
    std::string safeName = name;
    for (auto &c : safeName) {
        if (c == '/' || c == '\\') c = '_';
    }

    if (create) {
        hSem = CreateSemaphoreA(NULL, initialValue, 2147483647, safeName.c_str());
        if (!hSem) {
            throw std::runtime_error("CreateSemaphoreA failed. Error: " + std::to_string(GetLastError()));
        }
        creator = true;
    } else {
        hSem = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, safeName.c_str());
        if (!hSem) {
            hSem = CreateSemaphoreA(NULL, 0, 2147483647, safeName.c_str());
            if (!hSem)
                throw std::runtime_error("OpenSemaphoreA failed. Error: " + std::to_string(GetLastError()));
        }
        creator = false;
    }
}

void SharedSemaphore::wait() {
    if (!hSem) throw std::runtime_error("Semaphore not initialized");
    WaitForSingleObject(hSem, INFINITE);
}

void SharedSemaphore::post() {
    if (!hSem) throw std::runtime_error("Semaphore not initialized");
    ReleaseSemaphore(hSem, 1, NULL);
}

#else
#include <cstring>
#include <unistd.h>
#include <new> 

SharedSemaphore::SharedSemaphore()
    : data(nullptr), creator(false) {}

SharedSemaphore::SharedSemaphore(const std::string& name, bool create, int initialValue)
{
    init(name, create, initialValue);
}

void SharedSemaphore::init(const std::string& name, bool create, int initialValue)
{
    if (data) {
        this->~SharedSemaphore();
        new (this) SharedSemaphore(); 
    }

    size_t sz = 4096;

    if (create) {
        if (!shm.create(name, sz))
            throw std::runtime_error("Failed to create semaphore segment");
        creator = true;
    } else {
        if (!shm.open(name, sz))
            throw std::runtime_error("Failed to open semaphore segment");
        creator = false;
    }

    data = reinterpret_cast<SemaphoreData*>(shm.getBuffer());

    if (!data)
        throw std::runtime_error("SemaphoreData mmap returned null");

    if (creator) {
        std::memset(data, 0, sizeof(SemaphoreData));

        pthread_mutexattr_t mAttr;
        pthread_condattr_t  cAttr;

        pthread_mutexattr_init(&mAttr);
        pthread_condattr_init(&cAttr);

        pthread_mutexattr_setpshared(&mAttr, PTHREAD_PROCESS_SHARED);
        pthread_condattr_setpshared(&cAttr, PTHREAD_PROCESS_SHARED);

        if (pthread_mutex_init(&data->mtx, &mAttr) != 0)
            throw std::runtime_error("Failed to init mutex");

        if (pthread_cond_init(&data->cond, &cAttr) != 0)
            throw std::runtime_error("Failed to init condvar");

        data->value = initialValue;

        pthread_mutexattr_destroy(&mAttr);
        pthread_condattr_destroy(&cAttr);
    }
}

SharedSemaphore::~SharedSemaphore() {
    shm.close();
    data = nullptr;
}


void SharedSemaphore::wait() {
    if (!data) return;
    pthread_mutex_lock(&data->mtx);

    while (data->value == 0)
        pthread_cond_wait(&data->cond, &data->mtx);

    data->value--;

    pthread_mutex_unlock(&data->mtx);
}

void SharedSemaphore::post() {
    if (!data) return;
    pthread_mutex_lock(&data->mtx);

    data->value++;
    pthread_cond_signal(&data->cond);

    pthread_mutex_unlock(&data->mtx);
}
#endif