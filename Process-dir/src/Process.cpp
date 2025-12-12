#include "../include/Process.h"
#include <stdexcept>
#include <sstream>
#include <iostream>
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

bool Process::startSockets(unsigned short basePort, SocketType type) {
    useSockets = true;

    if (!stdinServer.create(type) || !stdoutServer.create(type) || !stderrServer.create(type))
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
        << " " << (type == SocketType::Unix ? "unix" : "ipv4")
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
    std::cerr << "[parent] accepted stdin client";
    stdoutClient = stdoutServer.acceptClient();
    std::cerr << "[parent] accepted stdout client\n";
    stderrClient = stderrServer.acceptClient();
    std::cerr << "[parent] accepted stderr client\n";


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
    useSharedMemory = false;

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

bool Process::startSockets(unsigned short basePort, SocketType type) {
    useSockets = true;
    useSharedMemory = false;

    if (!stdinServer.create(type) || !stdoutServer.create(type) || !stderrServer.create(type))
        throw std::runtime_error("socket create failed");

    if (!stdinServer.bindAndListen(basePort) ||
        !stdoutServer.bindAndListen(basePort + 1) ||
        !stderrServer.bindAndListen(basePort + 2))
        throw std::runtime_error("bind/listen failed");

    pid = fork();
    if (pid < 0) throw std::runtime_error("fork failed");

    if (pid == 0) {
        std::string domain = (type == SocketType::Unix) ? "unix" : "ipv4";
        std::string p0 = std::to_string(basePort);
        std::string p1 = std::to_string(basePort + 1);
        std::string p2 = std::to_string(basePort + 2);

        std::vector<std::string> allArgs;
        allArgs.push_back(domain);
        allArgs.push_back(p0);
        allArgs.push_back(p1);
        allArgs.push_back(p2);
        allArgs.insert(allArgs.end(), arguments.begin(), arguments.end());

        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (auto& a : allArgs)
            argv.push_back(const_cast<char*>(a.data()));
        argv.push_back(nullptr);

        if (pid == 0) {
            execvp(executable.c_str(), argv.data());
            perror("execvp failed");
            _exit(127);
        }
    }

    // accept connections from child
    stdinClient  = stdinServer.acceptClient();
    std::cerr << "[parent] accepted stdin client";
    stdoutClient = stdoutServer.acceptClient();
    std::cerr << "[parent] accepted stdout client\n";
    stderrClient = stderrServer.acceptClient();
    std::cerr << "[parent] accepted stderr client\n";


    return true;
}

bool Process::startSharedMemory(size_t size) {
    useSharedMemory = true;
    shmSize = size;

    int parentPid = getpid();

    std::string shmInName  = "/proc_shm_in_"  + std::to_string(parentPid);
    std::string shmOutName = "/proc_shm_out_" + std::to_string(parentPid);
    std::string semInName  = "/proc_sem_in_"  + std::to_string(parentPid);
    std::string semOutName = "/proc_sem_out_" + std::to_string(parentPid);

    // Create shared memory
    if (!shmIn.create(shmInName, size))
        throw std::runtime_error("Failed to create shmIn");

    if (!shmOut.create(shmOutName, size))
        throw std::runtime_error("Failed to create shmOut");

    // Create semaphores (parent only)
    semIn.init(semInName, true, 0);
    semOut.init(semOutName, true, 0);

    pid = fork();
    if (pid < 0)
        throw std::runtime_error("fork failed");

    // CHILD
    if (pid == 0) {

        std::vector<char*> argv;

        argv.push_back(const_cast<char*>(executable.c_str()));

        // pass names to child
        argv.push_back(const_cast<char*>(shmInName.c_str()));
        argv.push_back(const_cast<char*>(shmOutName.c_str()));
        argv.push_back(const_cast<char*>(semInName.c_str()));
        argv.push_back(const_cast<char*>(semOutName.c_str()));

        // pass user arguments
        for (auto &a : arguments)
            argv.push_back(const_cast<char*>(a.c_str()));

        argv.push_back(nullptr);

        execvp(executable.c_str(), argv.data());
        perror("execvp failed");
        _exit(127);
    }

    return true;
}

int Process::wait() {
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

void Process::writeStdin(const std::string& s) {
    if (useSharedMemory) {
        shmIn.write(s);
        semIn.post();
        return;
    }

    if (useSockets)
        stdinClient.write(s);
    else
        stdinPipe.write(s);
}

std::string Process::readStdout() {
    if (useSharedMemory) {
        semOut.wait();
        return shmOut.read();
    }

    if (useSockets)
        return stdoutClient.readAll();

    return stdoutPipe.readAll();
}

std::string Process::readStderr() {
    if (useSharedMemory)
        return "";

    if (useSockets)
        return stderrClient.readAll();

    return stderrPipe.readAll();
}

void Process::closeStdin() {
    if (useSharedMemory)
        return;

    if (useSockets)
        stdinClient.close();
    else
        stdinPipe.closeWrite();
}

void Process::terminate() {
    if (pid > 0) kill(pid, SIGKILL);
}

#endif
