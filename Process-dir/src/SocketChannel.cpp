//
// Created by Дмитро on 17.11.2025.
//
#include "../include/SocketChannel.h"
#include <stdexcept>

#ifdef _WIN32
bool SocketChannel::wsaStarted = false;

void SocketChannel::ensureWSAStarted() {
    if (!wsaStarted) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            throw std::runtime_error("WSAStartup failed");
        wsaStarted = true;
    }
}
#endif

SocketChannel::SocketChannel() {
#ifdef _WIN32
    sock = INVALID_SOCKET;
#else
    sock = -1;
#endif
}

SocketChannel::~SocketChannel() {
    close();
}

SocketChannel::SocketChannel(SocketChannel&& other) noexcept {
#ifdef _WIN32
    sock = other.sock;
    other.sock = INVALID_SOCKET;
#else
    sock = other.sock;
    other.sock = -1;
#endif
}

SocketChannel& SocketChannel::operator=(SocketChannel&& other) noexcept {
    if (this != &other) {
        close();
#ifdef _WIN32
        sock = other.sock;
        other.sock = INVALID_SOCKET;
#else
        sock = other.sock;
        other.sock = -1;
#endif
    }
    return *this;
}

bool SocketChannel::create() {
#ifdef _WIN32
    ensureWSAStarted();
    sock = ::socket(AF_INET, SOCK_STREAM, 0);
    return sock != INVALID_SOCKET;
#else
    sock = ::socket(AF_INET, SOCK_STREAM, 0);
    return sock != -1;
#endif
}

bool SocketChannel::bindAndListen(unsigned short port, int backlog) {
#ifdef _WIN32
    if (sock == INVALID_SOCKET) return false;
#else
    if (sock == -1) return false;
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port);

    int opt = 1;
#ifdef _WIN32
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    if (::bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return false;
    if (::listen(sock, backlog) == SOCKET_ERROR) return false;
#else
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (::bind(sock, (sockaddr*)&addr, sizeof(addr)) == -1) return false;
    if (::listen(sock, backlog) == -1) return false;
#endif
    return true;
}

SocketChannel SocketChannel::acceptClient() {
    SocketChannel c;
#ifdef _WIN32
    SOCKET s = ::accept(sock, nullptr, nullptr);
    if (s == INVALID_SOCKET) return c;
    c.sock = s;
#else
    int s = ::accept(sock, nullptr, nullptr);
    if (s == -1) return c;
    c.sock = s;
#endif
    return std::move(c);
}

bool SocketChannel::connectTo(const std::string& host, unsigned short port) {
#ifdef _WIN32
    ensureWSAStarted();
    if (sock == INVALID_SOCKET) return false;
#else
    if (sock == -1) return false;
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) return false;

#ifdef _WIN32
    return ::connect(sock, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR;
#else
    return ::connect(sock, (sockaddr*)&addr, sizeof(addr)) != -1;
#endif
}

void SocketChannel::close() {
#ifdef _WIN32
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
#else
    if (sock != -1) {
        ::close(sock);
        sock = -1;
    }
#endif
}

std::string SocketChannel::readAll() {
    std::string result;
#ifdef _WIN32
    if (sock == INVALID_SOCKET) return result;
#else
    if (sock == -1) return result;
#endif

    char buf[4096];
    for (;;) {
#ifdef _WIN32
        int n = ::recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) break;
#else
        ssize_t n = ::recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) break;
#endif
        result.append(buf, n);
    }
    return result;
}

void SocketChannel::write(const std::string& data) {
#ifdef _WIN32
    if (sock == INVALID_SOCKET) return;
#else
    if (sock == -1) return;
#endif

    const char* p = data.data();
    size_t left = data.size();
    while (left > 0) {
#ifdef _WIN32
        int n = ::send(sock, p, (int)left, 0);
        if (n <= 0) break;
#else
        ssize_t n = ::send(sock, p, left, 0);
        if (n <= 0) break;
#endif
        left -= (size_t)n;
        p += n;
    }
}