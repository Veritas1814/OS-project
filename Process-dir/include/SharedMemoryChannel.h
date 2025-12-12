#pragma once
#include <string>

class SharedMemoryChannel {
public:
    SharedMemoryChannel();
    ~SharedMemoryChannel();

    bool create(const std::string& name, size_t size);
    bool open(const std::string& name, size_t size);

    bool write(const std::string& data);
    std::string read();

    void close();

    void* getBuffer() const { return buffer; }
    size_t getSize() const { return size; }

private:
#ifdef _WIN32
    void* hMap = nullptr;
    void* buffer = nullptr;
#else
    int fd = -1;
    void* buffer = nullptr;
#endif

    size_t size = 0;
    std::string name;
};