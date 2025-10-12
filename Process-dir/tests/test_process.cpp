// This is a demo version of PVS-Studio for educational use.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include <iostream>
#include "../include/Process.h"

int main() {
#ifdef _WIN32
    std::cout << "Windows tests:\n";
    {
        std::cout << "Test 1: echo(Hello)\n";
        Process p("cmd", {"/C", "echo Hello"});
        if (!p.start()) {
            std::cerr << "Failed to start process\n";
            return 1;
        }
        int code = p.wait();
        std::cout << "stdout:\n" << p.readStdout();
        std::cout << "exit code: " << code << "\n\n";
    }

    {
        std::cout << "Test 2: findstr with stdin\n";
        Process p("findstr", {"Hello"});
        if (!p.start()) {
            std::cerr << "Failed to start process\n";
            return 1;
        }
        p.writeStdin("Line1\nLine2\n");
        p.writeStdin("Line3\n");
        p.closeStdin();
        int code = p.wait();
        std::cout << "stdout:\n" << p.readStdout();
        std::cout << "exit code: " << code << "\n\n";
    }
{
        std::cout << "Test 3: Invalid command\n";
        Process p("nonexistentcommand", {});
        if (!p.start()) {
            std::cerr << "Failed to start process as expected\n";
        } else {
            int code = p.wait();
            std::cout << "stdout:\n" << p.readStdout();
            std::cout << "exit code: " << code << "\n\n";
        }
    }

    {
        std::cout << "Test 4: Large input via stdin\n";
        Process p("findstr", {"Test"});
        if (!p.start()) { std::cerr << "Failed to start process\n"; return 1; }
        std::string bigInput(100000, 'A');
        bigInput += "Test\n";
        p.writeStdin(bigInput);
        p.closeStdin();
        int code = p.wait();
        std::cout << "stdout size: " << p.readStdout().size() << "\n";
        std::cout << "exit code: " << code << "\n\n";
    }

    {
        std::cout << "Test 5: Long running process (timeout simulation)\n";
        Process p("ping", {"127.0.0.1", "-n", "3"}); // Windows ping 3 times
        if (!p.start()) { std::cerr << "Failed to start process\n"; return 1; }
        int code = p.wait();
        std::cout << "stdout:\n" << p.readStdout();
        std::cout << "exit code: " << code << "\n\n";
    }

#else
    std::cout << "POSIX tests:\n";

    {
        std::cout << "Test 1: echo (Correct : Hello)\n";
        Process p("/bin/echo", {"Hello"});
        if (!p.start()) { std::cerr << "Failed to start process\n"; return 1; }
        int code = p.wait();
        std::cout << "stdout:\n" << p.readStdout();
        std::cout << "exit code: " << code << "\n\n";
    }

    {
        std::cout << "Test 2: cat with stdin(correct: Line1/Line2)\n";
        Process p("/bin/cat", {});
        if (!p.start()) { std::cerr << "Failed to start process\n"; return 1; }
        p.writeStdin("Line1\nLine2\n");
        p.writeStdin("Line3\n");
        p.closeStdin();
        int code = p.wait();
        std::cout << "stdout:\n" << p.readStdout();
        std::cout << "exit code: " << code << "\n\n";
    }

    {
        std::cout << "Test 3: Invalid command\n";
        Process p("/bin/nonexistentcmd", {});
        if (!p.start()) {
            std::cerr << "Failed to start process as expected\n";
        } else {
            int code = p.wait();
            std::cout << "stdout:\n" << p.readStdout();
            std::cout << "exit code: " << code << "\n\n";
        }
    }

    {
        std::cout << "Test 4: Large input via stdin\n";
        Process p("/bin/cat", {});
        if (!p.start()) { std::cerr << "Failed to start process\n"; return 1; }
        std::string bigInput(100000, 'A');
        bigInput += "Test\n";
        p.writeStdin(bigInput);
        p.closeStdin();
        int code = p.wait();
        std::cout << "stdout size: " << p.readStdout().size() << "\n";
        std::cout << "exit code: " << code << "\n\n";
    }

    {
        std::cout << "Test 5: Long running process\n";
        Process p("/bin/sleep", {"2"}); // Sleep 2 seconds
        if (!p.start()) { std::cerr << "Failed to start process\n"; return 1; }
        int code = p.wait();
        std::cout << "stdout:\n" << p.readStdout();
        std::cout << "exit code: " << code << "\n\n";
    }

    {
        std::cout << "Test 6: Process with no output\n";
        Process p("/bin/true", {}); // true exits 0, no output
        if (!p.start()) { std::cerr << "Failed to start process\n"; return 1; }
        int code = p.wait();
        std::cout << "stdout:\n'" << p.readStdout() << "'\n";
        std::cout << "exit code: " << code << "\n\n";
    }

#endif

    std::cout << "All tests done.\n";
    return 0;
}
