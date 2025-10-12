// This is a demo version of PVS-Studio for educational use.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include "../include/Process.h"
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

    HANDLE hStdOutWrite, hStdErrWrite, hStdInRead;

    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttr, 0)) return false;
    if (!CreatePipe(&hStdErrRead, &hStdErrWrite, &saAttr, 0)) return false;
    if (!CreatePipe(&hStdInRead, &hStdInWrite, &saAttr, 0)) return false;

    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStdErrRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(STARTUPINFOA);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdErrWrite;
    si.hStdInput = hStdInRead;

    std::ostringstream cmd;
    cmd << "\"" << executable << "\"";
    for (auto& a : arguments) cmd << " " << a;

    BOOL success = CreateProcessA(
        nullptr,
        const_cast<char*>(cmd.str().c_str()),
        nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi
    );

    // закриваємо кінці, які не потрібні батьківському процесу
    CloseHandle(hStdOutWrite);
    CloseHandle(hStdErrWrite);
    CloseHandle(hStdInRead);

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

void Process::writeStdin(const std::string& input) {
    if (hStdInWrite) {
        DWORD written;
        WriteFile(hStdInWrite, input.c_str(), static_cast<DWORD>(input.size()), &written, nullptr);
    }
}

void Process::closeStdin() {
    if (hStdInWrite) {
        CloseHandle(hStdInWrite);
        hStdInWrite = nullptr;
    }
}

#else  // POSIX: macOS, Linux

#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include <fcntl.h>


Process::Process(const std::string& path, const std::vector<std::string>& args)
    : executable(path), arguments(args) {
    stdoutPipe[0] = stdoutPipe[1] = -1;
    stderrPipe[0] = stderrPipe[1] = -1;
    stdinPipe[0] = stdinPipe[1] = -1;
}

bool Process::start() {
    if (pipe(stdoutPipe) < 0 || pipe(stderrPipe) < 0 || pipe(stdinPipe) < 0)
        throw std::runtime_error("pipe failed");

    pid = fork();
    if (pid < 0) throw std::runtime_error("fork failed");

    if (pid == 0) {
        // child
        dup2(stdoutPipe[1], STDOUT_FILENO);
        dup2(stderrPipe[1], STDERR_FILENO);
        dup2(stdinPipe[0], STDIN_FILENO);

        // закриваємо зайві кінці
        close(stdoutPipe[0]);
        close(stderrPipe[0]);
        close(stdinPipe[1]); // <-- обов’язково!

        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (auto& a : arguments)
            argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);

        execvp(executable.c_str(), argv.data());
        _exit(127);
    }

    // parent
    close(stdoutPipe[1]);
    close(stderrPipe[1]);
    close(stdinPipe[0]);
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

void Process::closeStdin() {
    if (stdinPipe[1] != -1) {
        close(stdinPipe[1]);
        stdinPipe[1] = -1;
    }
}


void Process::writeStdin(const std::string& input) {
    if (stdinPipe[1] != -1) {
        size_t totalWritten = 0;
        const char* buffer = input.c_str();
        size_t toWrite = input.size();

        while (totalWritten < toWrite) {
            ssize_t written = write(stdinPipe[1], buffer + totalWritten, toWrite - totalWritten);
            if (written < 0) {
                if (errno == EINTR) {
                    continue; // Interrupted by signal, retry
                }
                perror("write to stdinPipe failed");
                break;
            }
            totalWritten += static_cast<size_t>(written);
        }
    }
}

#endif