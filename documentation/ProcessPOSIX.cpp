#include <Process.h>
#include <iostream>

int main() {
    // 1. Define the process to run.
    // "/bin/ls" is the executable, "-la" is the argument.
    Process p("/bin/ls", {"-la"});

    try {
        // 2. Start the process
        p.start();

        // 3. Wait for it to finish
        int exitCode = p.wait();

        // 4. Get the output
        std::string output = p.readStdout();

        std::cout << "Exit Code: " << exitCode << "\n";
        std::cout << "Output:\n" << output << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
    return 0;
}