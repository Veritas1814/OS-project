#include "../include/Process.h"
#include <stdexcept>
#include <sstream>

#ifdef _WIN32
#include <windows.h>

Process::Process(const std::string& path, const std::vector<std::string>& args)
    : executable(path), arguments(args) {}

bool Process::start() {
    useSockets = false;

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

    CloseHandle(stdinPipe.getWriteHandle());
    CloseHandle(stdoutPipe.getReadHandle());
    CloseHandle(stderrPipe.getReadHandle());

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

#else

#include <unistd.h>
#include <sys/wait.h>
#include <csignal>

Process::Process(const std::string& path, const std::vector<std::string>& args)
    : executable(path), arguments(args) {}

bool Process::start() {
    useSockets = false;

    if (!stdinPipe.create() || !stdoutPipe.create() || !stderrPipe.create())
        throw std::runtime_error("Pipe creation failed");

    pid = fork();
    if (pid < 0) throw std::runtime_error("fork failed");

    if (pid == 0) {
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

    stdoutPipe.closeWrite();
    stderrPipe.closeWrite();
    stdinPipe.closeRead();

    return true;
}

bool Process::startSockets(unsigned short basePort) {
    useSockets = true;

    if (!stdinServer.create() || !stdoutServer.create() || !stderrServer.create())
        throw std::runtime_error("socket create failed");

    if (!stdinServer.bindAndListen(basePort) ||
        !stdoutServer.bindAndListen(basePort + 1) ||
        !stderrServer.bindAndListen(basePort + 2))
        throw std::runtime_error("bind/listen failed");

    pid = fork();
    if (pid < 0) throw std::runtime_error("fork failed");

    if (pid == 0) {
        std::string p0 = std::to_string(basePort);
        std::string p1 = std::to_string(basePort + 1);
        std::string p2 = std::to_string(basePort + 2);

        std::vector<std::string> allArgs;
        allArgs.push_back(p0);
        allArgs.push_back(p1);
        allArgs.push_back(p2);
        allArgs.insert(allArgs.end(), arguments.begin(), arguments.end());

        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (auto& a : allArgs)
            argv.push_back(const_cast<char*>(a.data()));
        argv.push_back(nullptr);

        if (execvp(executable.c_str(), argv.data()) == -1) {
            perror("execvp failed");
            _exit(127);
        }
    }

    stdinClient  = stdinServer.acceptClient();
    stdoutClient = stdoutServer.acceptClient();
    stderrClient = stderrServer.acceptClient();

    return true;
}

int Process::wait() {
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

std::string Process::readStdout() {
    if (useSockets)
        return stdoutClient.readAll();
    return stdoutPipe.readAll();
}

std::string Process::readStderr() {
    if (useSockets)
        return stderrClient.readAll();
    return stderrPipe.readAll();
}

void Process::writeStdin(const std::string& input) {
    if (useSockets)
        stdinClient.write(input);
    else
        stdinPipe.write(input);
}

void Process::closeStdin() {
    if (useSockets)
        stdinClient.close();
    else
        stdinPipe.closeWrite();
}

void Process::terminate() {
    if (pid > 0) kill(pid, SIGKILL);
}

#endif