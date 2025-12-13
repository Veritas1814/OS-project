#include <Pipe.h>
#include <iostream>
#include <thread>
#include <chrono>

void writerThread(Pipe* p) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // Write data into the pipe
    p->write("Hello from Unix Thread!");
    p->closeWrite(); // Close write end to signal EOF
}

int main() {
    Pipe myPipe;

    // 1. Create the POSIX pipe (pipe())
    if (!myPipe.create()) {
        std::cerr << "Failed to create pipe\n";
        return 1;
    }

    std::thread t(writerThread, &myPipe);

    std::cout << "Reading from pipe...\n";
    // readAll() calls read() until it gets 0 (EOF)
    std::string data = myPipe.readAll();

    std::cout << "Received: " << data << "\n";

    t.join();
    return 0;
}