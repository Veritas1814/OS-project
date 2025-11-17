#include "../include/SocketChannel.h"
#include <iostream>
#include <string>
#include <unistd.h>

int main(int argc, char** argv) {

    usleep(200000);

    if (argc < 4) {
        std::cerr << "need 3 ports\n";
        return 1;
    }

    std::cerr << "\n=== CHILD ARGUMENTS ===\n";
    std::cerr << "argc = " << argc << "\n";
    for (int i = 0; i < argc; i++) {
        std::cerr << "argv[" << i << "] = \"" << argv[i] << "\"\n";
    }
    std::cerr << "========================\n\n";

    int portIn  = std::stoi(argv[1]);
    int portOut = std::stoi(argv[2]);
    int portErr = std::stoi(argv[3]);

    SocketChannel in, out, err;

    // *** ВАЖЛИВО ***
    in.create();
    out.create();
    err.create();

    auto connect_retry = [&](SocketChannel& s, int port, const char* name) {
        for (int i = 0; i < 200; i++) {
            if (s.connectTo("127.0.0.1", port)) {
                return true;
            }
            usleep(20000);
        }
        std::cerr << "[child] failed to connect to " << name << "\n";
        return false;
    };

    if (!connect_retry(in,  portIn,  "stdin socket"))  return 2;
    if (!connect_retry(out, portOut, "stdout socket")) return 3;
    if (!connect_retry(err, portErr, "stderr socket")) return 4;

    std::string data = in.readAll();
    out.write("STDOUT FROM CHILD: " + data);
    err.write("STDERR FROM CHILD: " + data);

    return 0;
}
