#pragma once
#include <string>
#include <vector>

#include "Pipe.h"
#include "SocketChannel.h"
#include "SharedMemoryChannel.h"
#include "SharedSemaphore.h"

class Process {
public:
    Process(const std::string& path, const std::vector<std::string>& args);

    bool start();  // pipes
    bool startSockets(unsigned short basePort, SocketType type = SocketType::Unix);
    bool startSharedMemory(size_t size = 4096);

    int wait();

    std::string readStdout();
    std::string readStderr();
    void writeStdin(const std::string& input);
    void closeStdin();
    void terminate();

private:
    std::string executable;
    std::vector<std::string> arguments;

#ifdef _WIN32
    HANDLE hProcess = nullptr;
    HANDLE hThread = nullptr;
#else
    pid_t pid = -1;
#endif

    // PIPE IPC
    Pipe stdinPipe, stdoutPipe, stderrPipe;

    // SOCKET IPC
    SocketChannel stdinServer;
    SocketChannel stdoutServer;
    SocketChannel stderrServer;

    SocketChannel stdinClient;
    SocketChannel stdoutClient;
    SocketChannel stderrClient;

    bool useSockets = false;


    bool useSharedMemory = false;
    size_t shmSize = 0;
    std::string shmBase;

    SharedMemoryChannel shmIn;
    SharedMemoryChannel shmOut;

    SharedSemaphore semIn;
    SharedSemaphore semOut;
};