#include <Process.h>
#include <iostream>
#include <thread>
#include <chrono>

void writerThread(Pipe* p) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // Write data into the pipe
    p->write("Hello from Writer Thread!");
    // Close the write handle so the reader knows we are done
    p->closeWrite();
}

int main() {
    Pipe myPipe;

    // 1. Create the Windows pipe (CreatePipe)
    if (!myPipe.create()) {
        std::cerr << "Failed to create pipe\n";
        return 1;
    }

    // 2. Launch a thread to write into the pipe
    std::thread t(writerThread, &myPipe);

    // 3. Read from the pipe in the main thread
    // This will block until data is available or pipe is closed
    std::cout << "Reading from pipe...\n";
    std::string data = myPipe.readAll();

    std::cout << "Received: " << data << "\n";

    t.join();
    return 0;
}