#include "../include/Process.h"
#include <iostream>
#include <string>
#include <vector>

// Helper to hold a port open
SocketChannel blocker;

int main() {
#ifdef _WIN32
    const std::string childExec = "child_socket";
#else
    const std::string childExec = "./child_socket";
#endif

    // 1. Setup "Port Already in Use" scenario
    // We bind to port 9300 so the test using 9300 will fail
    if (!blocker.create(SocketType::IPv4) || !blocker.bindAndListen(9300)) {
        std::cerr << "SETUP ERROR: Could not bind blocking port 9300\n";
        return 1;
    }

    struct Test {
        SocketType type;
        unsigned short basePort;
        std::string name;
        std::string mode; // "echo", "invalid", "refuse"
        std::string extraHost;
        bool expectStartFail; // New flag for bind failure
    };

    std::vector<Test> tests = {
        // Happy Paths
        { SocketType::Unix, 9000, "Unix domain", "echo", "", false },
        { SocketType::IPv4, 9100, "IPv4 (127.0.0.1)", "echo", "", false },

        // Error Paths
        { SocketType::IPv4, 9200, "IPv4 invalid IP", "invalid", "256.256.256.256", false },

        // NEW: Connection Refused
        // We pass "REFUSE" as host to tell child to connect to a wrong (closed) port
        { SocketType::IPv4, 9150, "Connection Refused", "refuse", "REFUSE", false },

        // NEW: Port Already in Use
        // We try to bind to 9300, which 'blocker' is already holding
        { SocketType::IPv4, 9300, "Port Already in Use", "echo", "", true }
    };

    int passed = 0, failed = 0;

    for (const auto &t : tests) {
        std::cout << "==== RUN TEST: " << t.name << " on base port " << t.basePort << " ====\n";

        // Logic: Process::startSockets automatically adds [domain, p0, p1, p2].
        // We ONLY add the host argument here.
        std::string hostArg = t.extraHost.empty() ? (t.type == SocketType::Unix ? "" : "127.0.0.1") : t.extraHost;
        Process p(childExec, { hostArg });

        bool started = false;
        try {
            p.startSockets(t.basePort, t.type);
            started = true;
        } catch (const std::exception &e) {
            std::cerr << "startSockets failed: " << e.what() << "\n";
            if (t.expectStartFail) {
                std::cout << "[TEST PASSED] (Expected start failure)\n";
                passed++;
            } else {
                std::cout << "[TEST FAILED] (Unexpected start failure)\n";
                failed++;
            }
            std::cout << "==== END TEST: " << t.name << " ====\n\n";
            continue;
        }

        if (t.expectStartFail && started) {
            std::cout << "[TEST FAILED] (Expected start failure but succeeded)\n";
            failed++;
            p.terminate();
            continue;
        }

        // --- Interaction Phase ---
        if (t.mode == "echo") {
            p.writeStdin(t.type == SocketType::Unix
                             ? "hello through UNIX sockets!\n"
                             : "hello through IPv4 sockets!\n");
        }

        p.closeStdin();
        int code = p.wait();
        std::string out = p.readStdout();
        std::string err = p.readStderr();

        std::cout << "CHILD EXIT CODE: " << code << "\n";
        if (!out.empty()) std::cout << "CHILD STDOUT:\n" << out << "\n";
        if (!err.empty()) std::cout << "CHILD STDERR:\n" << err << "\n";

        bool success = false;
        if (t.mode == "echo") {
            success = (code == 0 && !out.empty() && !err.empty());
        }
        else if (t.mode == "invalid" || t.mode == "refuse") {
            // Should exit with non-zero (connection failed)
            success = (code != 0);
        }

        if (success) {
            std::cout << "[TEST PASSED]\n";
            ++passed;
        } else {
            std::cout << "[TEST FAILED]\n";
            ++failed;
        }

        std::cout << "==== END TEST: " << t.name << " ====\n\n";
    }

    std::cout << "==== TEST SUMMARY ====\n"
              << "Total tests: " << tests.size() << "\n"
              << "Passed:      " << passed << "\n"
              << "Failed:      " << failed << "\n";

    return failed == 0 ? 0 : 1;
}