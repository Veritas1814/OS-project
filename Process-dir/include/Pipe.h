#pragma once
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

class Pipe {
public:
    Pipe();
    ~Pipe();

    bool create();
    void closeRead();
    void closeWrite();
    std::string readAll();
    void write(const std::string& data);

#ifdef _WIN32
    HANDLE getReadHandle() const { return hRead; }
    HANDLE getWriteHandle() const { return hWrite; }
#else
    int getReadFD() const { return fdRead; }
    int getWriteFD() const { return fdWrite; }
#endif

private:
#ifdef _WIN32
    HANDLE hRead{nullptr};
    HANDLE hWrite{nullptr};
#else
    int fdRead{-1};
    int fdWrite{-1};
#endif
};
