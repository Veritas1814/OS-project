#include "../include/Process.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
#ifdef _WIN32
    const std::string childExec = "child_socket";
#else
    const std::string childExec = "./child_socket";
#endif

    struct Test { SocketType type; unsigned short basePort; std::string name; };
    std::vector<Test> tests = {
        { SocketType::Unix,  9000, "Unix domain" },
        { SocketType::IPv4,  9100, "IPv4 (127.0.0.1)" }
    };

    for (auto &t : tests) {
        std::cout << "==== RUN TEST: " << t.name << " on base port " << t.basePort << " ====\n";

        Process p(childExec, {});

        try {
            p.startSockets(t.basePort, t.type);
        } catch (const std::exception &e) {
            std::cerr << "startSockets failed: " << e.what() << "\n";
            continue;
        }

        if (t.type == SocketType::Unix)
            p.writeStdin("hello through UNIX sockets!\n");
        else
            p.writeStdin("hello through IPv4 sockets!\n");

        p.closeStdin();

        int code = p.wait();

        std::string out = p.readStdout();
        std::string err = p.readStderr();

        std::cout << "CHILD EXIT CODE: " << code << "\n";
        std::cout << "CHILD STDOUT:\n" << out << "\n";
        std::cout << "CHILD STDERR:\n" << err << "\n";
        std::cout << "==== END TEST: " << t.name << " ====\n\n";
    }

    return 0;
}
