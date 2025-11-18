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

    SetHandleInformation(stdinPipe.getWriteHandle(), HANDLE_FLAG_INHERIT, 0);

    SetHandleInformation(stdoutPipe.getReadHandle(), HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stderrPipe.getReadHandle(), HANDLE_FLAG_INHERIT, 0);
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
        TRUE,   // inherit handles
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

bool Process::startSockets(unsigned short basePort) {
    useSockets = true;

    if (!stdinServer.create() || !stdoutServer.create() || !stderrServer.create())
        throw std::runtime_error("socket create failed");

    if (!stdinServer.bindAndListen(basePort) ||
        !stdoutServer.bindAndListen(basePort + 1) ||
        !stderrServer.bindAndListen(basePort + 2))
        throw std::runtime_error("bind/listen failed");

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(STARTUPINFOA);

    std::ostringstream cmd;
    cmd << "\"" << executable << "\""
        << " " << basePort
        << " " << (basePort + 1)
        << " " << (basePort + 2);
    for (auto& a : arguments)
        cmd << " " << a;

    BOOL success = CreateProcessA(
        nullptr,
        const_cast<char*>(cmd.str().c_str()),
        nullptr,
        nullptr,
        FALSE,  
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!success)
        throw std::runtime_error("CreateProcessA failed");

    hProcess = pi.hProcess;
    hThread  = pi.hThread;

    stdinClient  = stdinServer.acceptClient();
    stdoutClient = stdoutServer.acceptClient();
    stderrClient = stderrServer.acceptClient();

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

bool Process::startSharedMemory(const std::string& baseName, size_t size) {
    useSharedMemory = true;
    shmBase = baseName;
    shmSize = size;

    if (!shmIn.create(baseName + "_in", size) ||
        !shmOut.create(baseName + "_out", size) ||
        !shmErr.create(baseName + "_err", size))
    {
        throw std::runtime_error("Failed to create shared memory regions");
    }

#ifdef _WIN32
    std::ostringstream cmd;
    cmd << "\"" << executable << "\" "
        << baseName << "_in "
        << baseName << "_out "
        << baseName << "_err";

    for (auto &a : arguments)
        cmd << " " << a;

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    BOOL ok = CreateProcessA(
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

    if (!ok)
        throw std::runtime_error("CreateProcessA failed (SHM)");

    hProcess = pi.hProcess;
    hThread = pi.hThread;

#else
    pid = fork();
    if (pid < 0)
        throw std::runtime_error("fork failed (SHM)");

    if (pid == 0) {
        std::string argIn  = baseName + "_in";
        std::string argOut = baseName + "_out";
        std::string argErr = baseName + "_err";

        std::vector<std::string> allArgs = {argIn, argOut, argErr};
        allArgs.insert(allArgs.end(), arguments.begin(), arguments.end());

        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(executable.c_str()));

        for (auto &s : allArgs)
            argv.push_back(const_cast<char*>(s.c_str()));

        argv.push_back(nullptr);

        execvp(executable.c_str(), argv.data());
        perror("execvp failed");
        _exit(127);
    }
#endif

    return true;
}

int Process::wait() {
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

void Process::writeStdin(const std::string& s) {
    if (useSharedMemory)
        shmIn.write(s);
    else if (useSockets)
        stdinClient.write(s);
    else
        stdinPipe.write(s);
}

std::string Process::readStdout() {
    if (useSharedMemory)
        return shmOut.read();
    if (useSockets)
        return stdoutClient.readAll();
    return stdoutPipe.readAll();
}

std::string Process::readStderr() {
    if (useSharedMemory)
        return shmErr.read();
    if (useSockets)
        return stderrClient.readAll();
    return stderrPipe.readAll();
}

void Process::closeStdin() {
    if (useSharedMemory)
        shmIn.close();
    else if (useSockets)
        stdinClient.close();
    else
        stdinPipe.closeWrite();
}

void Process::terminate() {
    if (pid > 0) kill(pid, SIGKILL);
}

#endif
