#include "../include/SharedSemaphore.h"
#include <stdexcept>
#include <cstring>
#include <unistd.h>

SharedSemaphore::SharedSemaphore()
    : data(nullptr), creator(false) {}

SharedSemaphore::SharedSemaphore(const std::string& name, bool create, int initialValue)
{
    // Виділяємо одну сторінку памʼяті під семафор
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

    // mmap вже здійснений всередині SharedMemoryChannel
    data = reinterpret_cast<SemaphoreData*>(shm.getBuffer());

    if (!data)
        throw std::runtime_error("SemaphoreData mmap returned null");

    // ---- ІНІЦІАЛІЗАЦІЯ ТІЛЬКИ В PARENT (create == true) ----
    if (creator) {
        // Обнуляємо весь блок памʼяті, щоб уникнути UB
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

void SharedSemaphore::init(const std::string& name, bool create, int initialValue)
{
    this->~SharedSemaphore();
    new (this) SharedSemaphore(name, create, initialValue);
}

SharedSemaphore::~SharedSemaphore() {
    shm.close();
}


void SharedSemaphore::wait() {
    pthread_mutex_lock(&data->mtx);

    while (data->value == 0)
        pthread_cond_wait(&data->cond, &data->mtx);

    data->value--;

    pthread_mutex_unlock(&data->mtx);
}

void SharedSemaphore::post() {
    pthread_mutex_lock(&data->mtx);

    data->value++;
    pthread_cond_signal(&data->cond);

    pthread_mutex_unlock(&data->mtx);
}