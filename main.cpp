#include "Process.h"
#include <iostream>

int main() {
#ifdef _WIN32
    Process p("cmd", {"/C", "echo Hello from Windows"});
#else
    Process p("/bin/echo", {"Hello from Unix"});
#endif

    p.start();
    int exitCode = p.wait();
    std::cout << "Output: " << p.readStdout() << "\n";
    std::cout << "Exit code: " << exitCode << "\n";
}
