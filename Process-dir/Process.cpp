#include "Process.h"
#include <stdexcept>
#include <sstream>

#ifdef _WIN32
#include <windows.h>

Process::Process(const std::string& path, const std::vector<std::string>& args)
    : executable(path), arguments(args) {}

bool Process::start() {
    SECURITY_ATTRIBUTES saAttr{};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = nullptr;

    HANDLE hStdOutWrite, hStdErrWrite;
    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttr, 0)) return false;
    if (!CreatePipe(&hStdErrRead, &hStdErrWrite, &saAttr, 0)) return false;
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStdErrRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(STARTUPINFOA);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdErrWrite;

    std::ostringstream cmd;
    cmd << "\"" << executable << "\"";
    for (auto& a : arguments) cmd << " " << a;

    BOOL success = CreateProcessA(
        nullptr,
        const_cast<char*>(cmd.str().c_str()),
        nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi
    );

    CloseHandle(hStdOutWrite);
    CloseHandle(hStdErrWrite);

    if (!success) return false;

    hProcess = pi.hProcess;
    hThread = pi.hThread;
    return true;
}

int Process::wait() {
    WaitForSingleObject(hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(hProcess, &code);
    CloseHandle(hProcess);
    CloseHandle(hThread);
    return static_cast<int>(code);
}

std::string Process::readStdout() {
    DWORD bytesRead;
    char buffer[4096];
    std::string output;
    while (ReadFile(hStdOutRead, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0)
        output.append(buffer, bytesRead);
    return output;
}

std::string Process::readStderr() {
    DWORD bytesRead;
    char buffer[4096];
    std::string output;
    while (ReadFile(hStdErrRead, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0)
        output.append(buffer, bytesRead);
    return output;
}

void Process::terminate() {
    if (hProcess) TerminateProcess(hProcess, 1);
}

#else  // POSIX: macOS, Linux

#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

Process::Process(const std::string& path, const std::vector<std::string>& args)
    : executable(path), arguments(args) {
    stdoutPipe[0] = stdoutPipe[1] = -1;
    stderrPipe[0] = stderrPipe[1] = -1;
}

bool Process::start() {
    if (pipe(stdoutPipe) < 0 || pipe(stderrPipe) < 0)
        throw std::runtime_error("pipe failed");

    pid = fork();
    if (pid < 0)
        throw std::runtime_error("fork failed");
    // child
    if (pid == 0) {
        dup2(stdoutPipe[1], STDOUT_FILENO);
        dup2(stderrPipe[1], STDERR_FILENO);
        close(stdoutPipe[0]);
        close(stderrPipe[0]);

        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (auto& a : arguments) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);

        execvp(executable.c_str(), argv.data());
        _exit(127);
    }

    close(stdoutPipe[1]);
    close(stderrPipe[1]);
    return true;
}

int Process::wait() {
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

std::string Process::readStdout() {
    char buffer[4096];
    ssize_t bytes;
    std::string output;
    while ((bytes = read(stdoutPipe[0], buffer, sizeof(buffer))) > 0)
        output.append(buffer, bytes);
    return output;
}

std::string Process::readStderr() {
    char buffer[4096];
    ssize_t bytes;
    std::string output;
    while ((bytes = read(stderrPipe[0], buffer, sizeof(buffer))) > 0)
        output.append(buffer, bytes);
    return output;
}

void Process::terminate() {
    if (pid > 0) kill(pid, SIGKILL);
}

#endif
