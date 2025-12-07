#include "../include/SocketChannel.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
static void sleep_ms(int ms) { Sleep(ms); }
#else
#include <unistd.h>
static void sleep_ms(int ms) { usleep(ms * 1000); }
#endif

int main(int argc, char** argv) {
    // startSockets sends: [1]domain, [2]p0, [3]p1, [4]p2.
    // User ctor arg sends: [5]host
    if (argc < 5) {
        std::cerr << "child_socket error: need domain + 3 ports\n";
        return 1;
    }

    std::string domain = argv[1];
    int portIn  = std::stoi(argv[2]);
    int portOut = std::stoi(argv[3]);
    int portErr = std::stoi(argv[4]);

    std::string hostArg = (argc > 5) ? argv[5] : "";

    SocketChannel in, out, err;
    SocketType type = (domain == "unix") ? SocketType::Unix : SocketType::IPv4;

    if (!in.create(type))  return 2;
    if (!out.create(type)) return 3;
    if (!err.create(type)) return 4;

    std::string targetHost = hostArg;

    // Logic for "Connection Refused" test
    if (hostArg == "REFUSE") {
        targetHost = "127.0.0.1";
        // Override ports to something likely closed to force refusal
        portIn = 55555;
        portOut = 55556;
        portErr = 55557;
    } else if (type == SocketType::IPv4 && targetHost.empty()) {
        targetHost = "127.0.0.1";
    }

    auto try_connect = [&](SocketChannel &s, int port, const char* name) -> bool {
        for (int i = 0; i < 5; ++i) {
            if (s.connectTo(targetHost, static_cast<unsigned short>(port))) return true;
            sleep_ms(50);
        }
        std::cerr << "[child] failed connect " << name << " (" << targetHost << ":" << port << ")\n";
        return false;
    };

    if (!try_connect(in,  portIn,  "stdin"))  return 5;
    if (!try_connect(out, portOut, "stdout")) return 6;
    if (!try_connect(err, portErr, "stderr")) return 7;

    std::string data = in.readAll();
    out.write("STDOUT FROM CHILD: " + data);
    err.write("STDERR FROM CHILD: " + data);

    return 0;
}