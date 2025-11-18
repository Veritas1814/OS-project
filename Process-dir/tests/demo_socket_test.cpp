#include "../include/Process.h"
#include <iostream>
#include <string>

int main() {
#ifdef _WIN32
    Process p("child_socket", {});
#else
    Process p("./child_socket", {});
#endif

    p.startSockets(9000);

    p.writeStdin("hello through sockets!\n");
    p.closeStdin();

    p.wait();

    std::string out = p.readStdout();
    std::string err = p.readStderr();

    std::cout << "CHILD STDOUT:\n" << out << "\n";
    std::cout << "CHILD STDERR:\n" << err << "\n";

    return 0;
}
