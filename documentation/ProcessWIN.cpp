#include <Process.h>
#include <iostream>

int main() {
    // 1. Define the process to run.
    // "cmd" is the executable, "/C" and "dir" are arguments.
    // This runs the Windows "dir" command.
    Process p("cmd", {"/C", "dir"});

    try {
        // 2. Start the process (defaults to using Pipes)
        p.start();

        // 3. Wait for the process to finish execution
        int exitCode = p.wait();

        // 4. Read everything the child process printed to Stdout
        std::string output = p.readStdout();

        std::cout << "Exit Code: " << exitCode << "\n";
        std::cout << "Output:\n" << output << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
    return 0;
}