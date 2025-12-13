#include <Process.h>        
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // --- CHILD LOGIC ---
    if (argc > 1) {
        unsigned short inPort  = static_cast<unsigned short>(std::stoi(argv[2]));
        unsigned short outPort = static_cast<unsigned short>(std::stoi(argv[3]));

        SocketChannel chIn, chOut;

        chIn.create(SocketType::IPv4); 
        if (!chIn.connectTo("127.0.0.1", inPort)) return 1;

        chOut.create(SocketType::IPv4);
        if (!chOut.connectTo("127.0.0.1", outPort)) return 1;

        std::string msg = chIn.readAll();
        std::string reply = "Socket Msg: " + msg;
        chOut.write(reply);

        chIn.close();
        chOut.close();
        return 0;
    }

    // --- PARENT LOGIC ---
    Process p(argv[0], {});

    try {
        p.startSockets(8000, SocketType::IPv4);

        p.writeStdin("Hello via TCP");
        p.closeStdin();

        std::string response = p.readStdout();

        p.wait();
        std::cout << "Parent received: " << response << "\n";
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}