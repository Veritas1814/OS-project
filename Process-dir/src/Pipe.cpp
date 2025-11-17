#include "../include/Pipe.h"
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

Pipe::Pipe() = default;

Pipe::~Pipe() {
    closeRead();
    closeWrite();
}

bool Pipe::create() {
#ifdef _WIN32
    SECURITY_ATTRIBUTES saAttr{};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = nullptr;

    if (!CreatePipe(&hRead, &hWrite, &saAttr, 0))
        return false;

    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hWrite, HANDLE_FLAG_INHERIT, 0);
    return true;
#else
    int fds[2];
    if (::pipe(fds) < 0) return false;
    readFD = fds[0];
    writeFD = fds[1];

    return true;
#endif
}

void Pipe::closeRead() {
#ifdef _WIN32
    if (hRead) {
        CloseHandle(hRead);
        hRead = nullptr;
    }
#else
    if (readFD != -1) {
        ::close(readFD);
        readFD = -1;
    }
#endif
}

void Pipe::closeWrite() {
#ifdef _WIN32
    if (hWrite) {
        CloseHandle(hWrite);
        hWrite = nullptr;
    }
#else
    if (writeFD != -1) {
        ::close(writeFD);
        writeFD = -1;
    }
#endif
}

std::string Pipe::readAll() {
    std::string result;
#ifdef _WIN32
    if (!hRead) return result;
    char buffer[4096];
    DWORD bytesRead;
    while (ReadFile(hRead, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0)
        result.append(buffer, bytesRead);
#else
    if (readFD == -1) return result;
    char buffer[4096];
    ssize_t bytes;
    while ((bytes = ::read(readFD, buffer, sizeof(buffer))) > 0)
        result.append(buffer, bytes);
#endif
    return result;
}

void Pipe::write(const std::string& data) {
#ifdef _WIN32
    if (!hWrite) return;
    DWORD written;
    WriteFile(hWrite, data.c_str(), static_cast<DWORD>(data.size()), &written, nullptr);
#else
    if (writeFD == -1) return;
    ::write(writeFD, data.c_str(), data.size());
#endif
}
