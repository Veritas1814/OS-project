#pragma once
#include <string>
#include <vector>

class Process {
public:
    Process(const std::string& path, const std::vector<std::string>& args);
    bool start();
    int wait();
    std::string readStdout();
    std::string readStderr();
    void terminate();

private:
    std::string executable;
    std::vector<std::string> arguments;

#ifdef _WIN32
    void* hProcess = nullptr;
    void* hThread = nullptr;
    void* hStdOutRead = nullptr;
    void* hStdErrRead = nullptr;
#else
    int pid = -1;
    int stdoutPipe[2];
    int stderrPipe[2];
#endif
};
