// This is a demo version of PVS-Studio for educational use.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include "../include/SocketChannel.h"
#include <stdexcept>
#include <iostream>
#include <cstdio>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <afunix.h>
#pragma comment(lib, "ws2_32.lib")

bool SocketChannel::wsaStarted = false;

static inline SOCKET to_native(socket_handle h) { return static_cast<SOCKET>(h); }
static inline socket_handle from_native(SOCKET s) { return static_cast<socket_handle>(s); }
static constexpr socket_handle INVALID_SOCKET_HANDLE = static_cast<socket_handle>(INVALID_SOCKET);

void SocketChannel::ensureWSAStarted() {
    if (!wsaStarted) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) throw std::runtime_error("WSAStartup failed");
        wsaStarted = true;
    }
}
#else // POSIX
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cerrno>
#include <cstring>
#include <sys/select.h>
#include <sys/time.h>

static inline int to_native(socket_handle h) { return static_cast<int>(h); }
static inline socket_handle from_native(int s) { return static_cast<socket_handle>(s); }
static constexpr socket_handle INVALID_SOCKET_HANDLE = -1;
#endif

static std::string make_unix_path(unsigned short port) {
#ifdef _WIN32
    return ".\\osproj_sock_" + std::to_string(port);
#else
    return "/tmp/osproj_sock_" + std::to_string(port);
#endif
}

SocketChannel::SocketChannel() : sock(INVALID_SOCKET_HANDLE), sockType(SocketType::Unix) {}
SocketChannel::~SocketChannel() { close(); }

SocketChannel::SocketChannel(SocketChannel&& other) noexcept : sock(other.sock), sockType(other.sockType) {
    other.sock = INVALID_SOCKET_HANDLE;
}

SocketChannel& SocketChannel::operator=(SocketChannel&& other) noexcept {
    if (this != &other) {
        close();
        sock = other.sock;
        sockType = other.sockType;
        other.sock = INVALID_SOCKET_HANDLE;
    }
    return *this;
}

bool SocketChannel::create(SocketType type) {
    sockType = type;
#ifdef _WIN32
    ensureWSAStarted();
#endif
    if (sockType == SocketType::Unix) {
#ifdef _WIN32
        SOCKET s = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (s == INVALID_SOCKET) return false;
        sock = from_native(s);
#else
        int s = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (s == -1) return false;
        sock = from_native(s);
#endif
    } else {
#ifdef _WIN32
        SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) return false;
        sock = from_native(s);
        
#else
        int s = ::socket(AF_INET, SOCK_STREAM, 0);
        if (s == -1) return false;
        sock = from_native(s);
        int opt = 1;
        ::setsockopt(to_native(sock), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    }
    return true;
}

bool SocketChannel::bindAndListen(unsigned short port, int backlog) {
    if (sock == INVALID_SOCKET_HANDLE) return false;

    if (sockType == SocketType::Unix) {
        std::string path = make_unix_path(port);
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path.c_str());
#ifdef _WIN32
        DeleteFileA(path.c_str());
        if (::bind(to_native(sock), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) return false;
#else
        ::unlink(path.c_str());
        if (::bind(to_native(sock), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) return false;
#endif
    } else {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);
#ifdef _WIN32
        if (::bind(to_native(sock), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) return false;
#else
        if (::bind(to_native(sock), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) return false;
#endif
    }

#ifdef _WIN32
    if (::listen(to_native(sock), backlog) == SOCKET_ERROR) return false;
#else
    if (::listen(to_native(sock), backlog) == -1) return false;
#endif
    return true;
}

SocketChannel SocketChannel::acceptClient() {
    SocketChannel c;
    c.sockType = sockType;
    if (sock == INVALID_SOCKET_HANDLE) return c;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(to_native(sock), &readfds);
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;

#ifdef _WIN32
    int ret = ::select(0, &readfds, nullptr, nullptr, &tv);
#else
    int ret = ::select(to_native(sock) + 1, &readfds, nullptr, nullptr, &tv);
#endif
    if (ret <= 0) {
        std::cerr << "[parent] accept timed out\n";
        return c;
    }

#ifdef _WIN32
    SOCKET s = ::accept(to_native(sock), nullptr, nullptr);
    if (s == INVALID_SOCKET) return c;
    c.sock = from_native(s);
#else
    int s = ::accept(to_native(sock), nullptr, nullptr);
    if (s == -1) return c;
    c.sock = from_native(s);
#endif
    return c;
}

bool SocketChannel::connectTo(const std::string& host, unsigned short port) {
    if (sock == INVALID_SOCKET_HANDLE) return false;

    if (sockType == SocketType::Unix) {
        std::string path = make_unix_path(port);
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path.c_str());
#ifdef _WIN32
        return ::connect(to_native(sock), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != SOCKET_ERROR;
#else
        return ::connect(to_native(sock), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != -1;
#endif
    } else {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
#ifdef _WIN32
        if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) return false;
        return ::connect(to_native(sock), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != SOCKET_ERROR;
#else
        if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) return false;
        return ::connect(to_native(sock), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != -1;
#endif
    }
}

void SocketChannel::close() {
    if (sock == INVALID_SOCKET_HANDLE) return;
#ifdef _WIN32
    ::closesocket(to_native(sock));
#else
    ::close(to_native(sock));
#endif
    sock = INVALID_SOCKET_HANDLE;
}

std::string SocketChannel::readAll() {
    std::string result;
    if (sock == INVALID_SOCKET_HANDLE) return result;
    char buf[4096];
    for (;;) {
#ifdef _WIN32
        int n = ::recv(to_native(sock), buf, sizeof(buf), 0);
#else
        ssize_t n = ::recv(to_native(sock), buf, sizeof(buf), 0);
#endif
        if (n <= 0) break;
        result.append(buf, static_cast<std::size_t>(n));
    }
    return result;
}

void SocketChannel::write(const std::string& data) {
    if (sock == INVALID_SOCKET_HANDLE) return;
    const char* p = data.data();
    std::size_t left = data.size();
    while (left > 0) {
#ifdef _WIN32
        int n = ::send(to_native(sock), p, static_cast<int>(left), 0);
#else
        ssize_t n = ::send(to_native(sock), p, left, 0);
#endif
        if (n <= 0) break;
        left -= static_cast<std::size_t>(n);
        p += n;
    }
}