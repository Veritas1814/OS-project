#pragma once
#include <string>
#include <vector>
#include "Pipe.h"

class Process {
public:
    Process(const std::string& path, const std::vector<std::string>& args);
    bool start();
    int wait();
    std::string readStdout();
    std::string readStderr();
    void writeStdin(const std::string& input);
    void terminate();
    void closeStdin();


private:
    std::string executable;
    std::vector<std::string> arguments;

#ifdef _WIN32
    void* hProcess = nullptr;
    void* hThread = nullptr;
    void* hStdOutRead = nullptr;
    void* hStdErrRead = nullptr;
    void* hStdInWrite = nullptr;
    Pipe stdinPipe;
    Pipe stdoutPipe;
    Pipe stderrPipe;
#else
    int pid = -1;
    Pipe stdinPipe;
    Pipe stdoutPipe;
    Pipe stderrPipe;
#endif
};