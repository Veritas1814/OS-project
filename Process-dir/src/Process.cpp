#include "../include/Process.h"
#include <stdexcept>
#include <sstream>

#ifdef _WIN32
#include <windows.h>

Process::Process(const std::string& path, const std::vector<std::string>& args)
    : executable(path), arguments(args) {}

bool Process::start() {
    if (!stdinPipe.create() || !stdoutPipe.create() || !stderrPipe.create())
        throw std::runtime_error("Pipe creation failed");

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(STARTUPINFOA);
    si.dwFlags |= STARTF_USESTDHANDLES;

    si.hStdInput  = stdinPipe.getReadHandle();
    si.hStdOutput = stdoutPipe.getWriteHandle();
    si.hStdError  = stderrPipe.getWriteHandle();

    std::ostringstream cmd;
    cmd << "\"" << executable << "\"";
    for (auto& a : arguments)
        cmd << " " << a;

    BOOL success = CreateProcessA(
        nullptr,
        const_cast<char*>(cmd.str().c_str()),
        nullptr,
        nullptr,
        TRUE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!success)
        throw std::runtime_error("CreateProcessA failed");

    CloseHandle(stdinPipe.getReadHandle());
    CloseHandle(stdoutPipe.getWriteHandle());
    CloseHandle(stderrPipe.getWriteHandle());

    hProcess = pi.hProcess;
    hThread  = pi.hThread;

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

std::string Process::readStdout() { return stdoutPipe.readAll(); }
std::string Process::readStderr() { return stderrPipe.readAll(); }

void Process::writeStdin(const std::string& input) { stdinPipe.write(input); }
void Process::closeStdin() { stdinPipe.closeWrite(); }

void Process::terminate() {
    if (hProcess) TerminateProcess(hProcess, 1);
}

#else // POSIX

#include <unistd.h>
#include <sys/wait.h>
#include <csignal>

Process::Process(const std::string& path, const std::vector<std::string>& args)
    : executable(path), arguments(args) {}

bool Process::start() {
    if (!stdinPipe.create() || !stdoutPipe.create() || !stderrPipe.create())
        throw std::runtime_error("Pipe creation failed");

    pid = fork();
    if (pid < 0) throw std::runtime_error("fork failed");

    if (pid == 0) {
        // child
        dup2(stdoutPipe.getWriteFD(), STDOUT_FILENO);
        dup2(stderrPipe.getWriteFD(), STDERR_FILENO);
        dup2(stdinPipe.getReadFD(), STDIN_FILENO);

        stdoutPipe.closeRead();
        stderrPipe.closeRead();
        stdinPipe.closeWrite();

        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (auto& a : arguments)
            argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);

        if (execvp(executable.c_str(), argv.data()) == -1) {
            perror("execvp failed");
            _exit(127);
        }
    }

    // parent
    stdoutPipe.closeWrite();
    stderrPipe.closeWrite();
    stdinPipe.closeRead();

    this->stdinPipe  = stdinPipe;
    this->stdoutPipe = stdoutPipe;
    this->stderrPipe = stderrPipe;

    return true;
}

int Process::wait() {
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

std::string Process::readStdout() { return stdoutPipe.readAll(); }
std::string Process::readStderr() { return stderrPipe.readAll(); }
void Process::writeStdin(const std::string& input) { stdinPipe.write(input); }
void Process::closeStdin() { stdinPipe.closeWrite(); }

void Process::terminate() {
    if (pid > 0) kill(pid, SIGKILL);
}

#endif
