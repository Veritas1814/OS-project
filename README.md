# What is this?
Cross-platform (Windows + POSIX) class library for working with processes and IPC\
Writing code that works on both Windows and Linux is usually hard because they use different system calls (like ```CreateProcess``` vs. ```fork()/exec()```). This library handles those differences for you, giving you a single, easy-to-use set of classes to launch programs and send data between them.
# Key Features
- Cross-Platform: Write your code once, and it runs on Windows and Linux/macOS.
- Easy Process Management: Launch child processes without worrying about low-level handles or PIDs.
- Multiple Ways to Communicate:
  - Pipes: Simple one-way data flow (Standard Input/Output).
  - Sockets: Network-style communication (TCP/IPv4 or Unix Domain Sockets).
  - Shared Memory: Fast data transfer by sharing RAM between programs.
- Synchronization Primitives: 
  - Cross-platform named semaphores for process synchronization.
# How to Use It
All the main classes are in the ipc namespace. You just need to include one file: ``IPC.h``
### 1. The `Process` Class
This is the main class you will use. It lets you run a program and talk to it.
#### Example 1: Simple Command (Using Pipes) This is the easiest way to run a command and get its output.
```
#include "include/IPC.h"
#include <iostream>

int main() {
    // 1. Create the process object (pick the command for your OS)
    #ifdef _WIN32
        ipc::Process p("cmd", {"/C", "echo Hello from Windows"});
    #else
        ipc::Process p("/bin/echo", {"Hello from Unix"});
    #endif

    // 2. Start the process
    p.start();

    // 3. Wait for it to finish and get the exit code
    int exitCode = p.wait();

    // 4. Print what the child process wrote
    std::cout << "Output: " << p.readStdout() << "\n";
    std::cout << "Exit code: " << exitCode << "\n";

    return 0;
}
```
