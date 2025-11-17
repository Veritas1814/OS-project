//
// Created by Дмитро on 17.11.2025.
//

#pragma once
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
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

#ifdef _WIN32
    SOCKET getSocket() const { return sock; }
#else
    int getFD() const { return sock; }
#endif

private:
#ifdef _WIN32
    SOCKET sock;
    static bool wsaStarted;
    static void ensureWSAStarted();
#else
    int sock;
#endif
};
