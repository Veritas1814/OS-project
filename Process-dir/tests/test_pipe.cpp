// This is a demo version of PVS-Studio for educational use.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include "../include/Pipe.h"
#include <iostream>

int main() {
    std::cout << "Pipe tests:\n";

    {
        std::cout << "Test 1: create + write/read\n";
        Pipe p;
        if (!p.create()) {
            std::cerr << "Failed to create pipe\n";
            return 1;
        }

        p.write("HelloPipe");
        p.closeWrite();
        std::string out = p.readAll();

        std::cout << "stdout:\n" << out << "\n";
        std::cout << "expected: HelloPipe\n\n";
    }

    {
        std::cout << "Test 2: read empty after closing write\n";
        Pipe p;
        if (!p.create()) {
            std::cerr << "Failed to create pipe\n";
            return 1;
        }

        p.closeWrite();
        std::string out = p.readAll();

        std::cout << "stdout:\n" << out << "\n";
        std::cout << "expected: (empty)\n\n";
    }

    {
        std::cout << "Test 3: multiple writes\n";
        Pipe p;
        if (!p.create()) {
            std::cerr << "Failed to create pipe\n";
            return 1;
        }

        p.write("A");
        p.write("B");
        p.write("C");
        p.closeWrite();
        std::string out = p.readAll();

        std::cout << "stdout:\n" << out << "\n";
        std::cout << "expected: ABC\n\n";
    }

    return 0;
}
