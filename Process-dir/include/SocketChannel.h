// SocketChannel.h
#pragma once

#include <string>
#include <cstdint>

#ifdef _WIN32
using socket_handle = std::uintptr_t;   // зберігаємо SOCKET як integer handle
#else
using socket_handle = int;              // звичайний fd
#endif
class SocketChannel {
public:
    SocketChannel();
    ~SocketChannel();

    SocketChannel(SocketChannel&& other) noexcept;
    SocketChannel& operator=(SocketChannel&& other) noexcept;
    SocketChannel(const SocketChannel&) = delete;
    SocketChannel& operator=(const SocketChannel&) = delete;

    bool create();
    bool bindAndListen(unsigned short port, int backlog = 1);
    SocketChannel acceptClient();
    bool connectTo(const std::string& host, unsigned short port);
    void close();
    std::string readAll();
    void write(const std::string& data);

private:
    socket_handle sock;

#ifdef _WIN32
    static bool wsaStarted;
    static void ensureWSAStarted();
#endif
};
