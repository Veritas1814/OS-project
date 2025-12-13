#include <Process.h>
#include <iostream>
#include <string>

// Note: On POSIX, Process.cpp sends "ipv4" or "unix" as argv[1]
int main(int argc, char* argv[]) {
    // --- CHILD LOGIC ---
    if (argc > 1) {
        // argv[2] is stdin port, argv[3] is stdout port
        unsigned short inPort  = (unsigned short)std::stoi(argv[2]);
        unsigned short outPort = (unsigned short)std::stoi(argv[3]);

        SocketChannel chIn, chOut;

        // Using Unix Domain Sockets (faster on Linux)
        chIn.create(SocketType::Unix);
        chOut.create(SocketType::Unix);

        // Unix sockets ignore IP, but connectTo signature needs it
        chIn.connectTo("127.0.0.1", inPort);
        chOut.connectTo("127.0.0.1", outPort);

        std::string msg = chIn.readAll();

        std::string response = "UnixSocket Echo: " + msg;
        chOut.write(response);

        chIn.close();
        chOut.close();
        return 0;
    }

    // --- PARENT LOGIC ---
    Process p(argv[0], {});

    try {
        // Use Unix Sockets with base port 9000
        p.startSockets(9000, SocketType::Unix);

        p.writeStdin("Data from Parent");
        p.closeStdin(); // Important: triggers EOF on child's read

        std::string output = p.readStdout();

        p.wait();
        std::cout << "Parent received: " << output << "\n";
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}