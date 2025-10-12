// This is a demo version of PVS-Studio for educational use.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include <iostream>
#include "../include/Process.h"

int main() {
#ifdef _WIN32
    std::cout << "Windows tests:\n";
    {
        std::cout << "Test 1: echo\n";
        Process p("cmd", {"/C", "echo Hello"});
        if (!p.start()) {
            std::cerr << "Failed to start process\n";
            return 1;
        }
        int code = p.wait();
        std::cout << "stdout: " << p.readStdout() << "\n";
        std::cout << "exit code: " << code << "\n\n";
    }

    {
        std::cout << "Test 2: findstr with stdin\n";
        Process p("findstr", {"Hello"});
        if (!p.start()) {
            std::cerr << "Failed to start process\n";
            return 1;
        }
        p.writeStdin("Hello world\nThis line won't match\n");
        p.writeStdin("Hello again\n");
        p.closeStdin();
        int code = p.wait();
        std::cout << "stdout: " << p.readStdout() << "\n";
        std::cout << "exit code: " << code << "\n\n";
    }

#else
    std::cout << "POSIX tests:\n";
    {
        std::cout << "Test 1: echo\n";
        Process p("/bin/echo", {"Hello"});
        if (!p.start()) {
            std::cerr << "Failed to start process\n";
            return 1;
        }
        int code = p.wait();
        std::cout << "stdout: " << p.readStdout() << "\n";
        std::cout << "exit code: " << code << "\n\n";
    }

    {
        std::cout << "Test 2: cat with stdin\n";
        Process p("/bin/cat", {});
        if (!p.start()) {
            std::cerr << "Failed to start process\n";
            return 1;
        }
        p.writeStdin("Line1\n");
        p.writeStdin("Line2\n");
        p.closeStdin();
        int code = p.wait();
        std::cout << "stdout: " << p.readStdout() << "\n";
        std::cout << "exit code: " << code << "\n\n";
    }

#endif

    std::cout << "All tests done.\n";
    return 0;
}
