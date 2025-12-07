// This is a demo version of PVS-Studio for educational use.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
#include "../include/SocketChannel.h"
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
static void sleep_ms(int ms) { Sleep(ms); }
#else
#include <unistd.h>
static void sleep_ms(int ms) { usleep(ms * 1000); }
#endif

int main(int argc, char** argv) {
    sleep_ms(200);

    if (argc < 5) {
        std::cerr << "need domain + 3 ports\n";
        return 1;
    }

    std::string domain = argv[1]; // "unix" or "ipv4"
    bool useUnix = (domain == "unix");

    std::cerr << "\n=== CHILD ARGUMENTS ===\n";
    std::cerr << "domain = " << domain << "\n";
    for (int i = 0; i < argc; i++) {
        std::cerr << "argv[" << i << "] = \"" << argv[i] << "\"\n";
    }
    std::cerr << "========================\n\n";

    int portIn  = std::stoi(argv[2]);
    int portOut = std::stoi(argv[3]);
    int portErr = std::stoi(argv[4]);

    SocketChannel in, out, err;

    SocketType type = useUnix ? SocketType::Unix : SocketType::IPv4;

    if (!in.create(type))  { std::cerr << "create(in) failed\n";  return 2; }
    if (!out.create(type)) { std::cerr << "create(out) failed\n"; return 3; }
    if (!err.create(type)) { std::cerr << "create(err) failed\n"; return 4; }

    auto try_connect = [&](SocketChannel &s, int port, const char* name) -> bool {
        if (useUnix) {
            for (int i = 0; i < 200; ++i) {
                if (s.connectTo("", static_cast<unsigned short>(port))) return true;
                sleep_ms(20);
            }
        } else {
            for (int i = 0; i < 200; ++i) {
                if (s.connectTo("127.0.0.1", static_cast<unsigned short>(port))) return true;
                sleep_ms(20);
            }
        }
        std::cerr << "[child] failed to connect to " << name << "\n";
        return false;
    };

    if (!try_connect(in,  portIn,  "stdin socket"))  return 5;
    if (!try_connect(out, portOut, "stdout socket")) return 6;
    if (!try_connect(err, portErr, "stderr socket")) return 7;

    std::string data = in.readAll();
    out.write("STDOUT FROM CHILD: " + data);
    err.write("STDERR FROM CHILD: " + data);

    return 0;
}
