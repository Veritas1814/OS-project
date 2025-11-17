#pragma once
#include <string>
#include <vector>
#include "Pipe.h"

#ifndef _WIN32
#include "SocketChannel.h"
#endif

class Process {
public:
    Process(const std::string& path, const std::vector<std::string>& args);

    bool start();
#ifndef _WIN32
    bool startSockets(unsigned short basePort);
#endif

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
    Pipe stdinPipe, stdoutPipe, stderrPipe;
#else
    pid_t pid = -1;
    Pipe stdinPipe, stdoutPipe, stderrPipe;
    SocketChannel stdinServer, stdoutServer, stderrServer;
    SocketChannel stdinClient, stdoutClient, stderrClient;
#endif

    bool useSockets = false;
};