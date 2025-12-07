#pragma once

#include <string>
#include <cstdint>

#ifdef _WIN32
using socket_handle = std::uintptr_t;
#else
using socket_handle = int;
#endif

enum class SocketType {
    Unix,
    IPv4
};

class SocketChannel {
public:
    SocketChannel();
    ~SocketChannel();

    SocketChannel(SocketChannel&& other) noexcept;
    SocketChannel& operator=(SocketChannel&& other) noexcept;
    SocketChannel(const SocketChannel&) = delete;
    SocketChannel& operator=(const SocketChannel&) = delete;

    bool create(SocketType type = SocketType::Unix);
    bool bindAndListen(unsigned short port, int backlog = 1);
    SocketChannel acceptClient();
    bool connectTo(const std::string& host, unsigned short port);
    void close();
    std::string readAll();
    void write(const std::string& data);

private:
    socket_handle sock;
    SocketType sockType{SocketType::Unix};

#ifdef _WIN32
    static bool wsaStarted;
    static void ensureWSAStarted();
#endif
};
