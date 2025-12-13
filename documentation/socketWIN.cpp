#include <Process.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // --- CHILD LOGIC ---
    if (argc > 1) {
        // Process.cpp passes: [1]=type, [2]=stdinPort, [3]=stdoutPort, [4]=stderrPort
        unsigned short inPort  = static_cast<unsigned short>(std::stoi(argv[2]));
        unsigned short outPort = static_cast<unsigned short>(std::stoi(argv[3]));

        SocketChannel chIn, chOut;

        // Child acts as Client. Connects to Parent.
        // Parent's Stdin is where Child READS from.
        chIn.create(SocketType::Tcp);
        if (!chIn.connectTo("127.0.0.1", inPort)) return 1;

        // Parent's Stdout is where Child WRITES to.
        chOut.create(SocketType::Tcp);
        if (!chOut.connectTo("127.0.0.1", outPort)) return 1;

        // Read from Parent
        std::string msg = chIn.readAll();

        // Write to Parent
        std::string reply = "Socket Msg: " + msg;
        chOut.write(reply);

        chIn.close();
        chOut.close();
        return 0;
    }

    // --- PARENT LOGIC ---
    Process p(argv[0], {});

    try {
        // Parent acts as Server.
        // Use port 8000 (Stdin), 8001 (Stdout), 8002 (Stderr)
        p.startSockets(8000, SocketType::Tcp);

        // writeStdin sends data via the first socket
        p.writeStdin("Hello via TCP");

        // Closes the first socket (sending EOF to child) so child's readAll() returns
        p.closeStdin();

        // readStdout reads from the second socket
        std::string response = p.readStdout();

        p.wait();
        std::cout << "Parent received: " << response << "\n";
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}