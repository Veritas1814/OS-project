#pragma once
#include <string>
#include <vector>
#include "Pipe.h"
#include "SocketChannel.h"

class Process {
public:
    Process(const std::string& path, const std::vector<std::string>& args);

    bool start();
    bool startSockets(unsigned short basePort, SocketType type = SocketType::Unix);

    int wait();

    std::string readStdout();
    std::string readStderr();
    void writeStdin(const std::string& input);
    void closeStdin();
    void terminate();

private:
    std::string executable;
    std::vector<std::string> arguments;

    std::string shmBase;
    size_t shmSize = 0;
#ifdef _WIN32
    HANDLE hProcess = nullptr;
    HANDLE hThread = nullptr;
#else
    pid_t pid = -1;
#endif

    SocketChannel stdinServer;
    SocketChannel stdoutServer;
    SocketChannel stderrServer;

    SocketChannel stdinClient;
    SocketChannel stdoutClient;
    SocketChannel stderrClient;

    Pipe stdinPipe, stdoutPipe, stderrPipe;
    bool useSockets = false;

    bool useSharedMemory = false;
};
