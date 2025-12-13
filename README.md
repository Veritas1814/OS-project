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
1. You need to clone our github repository 
2. If you are on Mac/Linux:
```bash
cmake -S . -B build
```
2. If you use Windows
```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="absolute/path/to/Os-project/install"
example
(cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Users/matvi/Code/OS-project/install")
```
3. For all OS run this
```bash
cmake --build build --config Release
```
4. Mac/Linux: (You don't need to do this on Windows)
```bash
sudo cmake --install build
```
1. Process\
`ProcessPOSIX, ProcessWIN` In this example, you can see how to spawn a new child process (like ls on Linux or dir on Windows) from your C++ application.
    - It demonstrates: How to launch an executable, pass command-line arguments, wait for it to finish, and capture its standard output (stdout) into a string.
2. Pipe\
`pipePOSIX, pipeWIN`In this example, you can see how to create a raw anonymous pipe to stream data between two points.
    - It demonstrates: Low-level one-way communication where one thread writes data ("Hello world") and another thread reads it continuously until the pipe is closed.
3. Shared Memory(SharedMemoryChannel)\
`SharedMemoryPOSIX, SharedMemoryWIN`In this example, you can see two separate processes (Parent and Child) accessing the exact same block of RAM to exchange data without copying it.
    - It demonstrates: The fastest possible IPC method. The parent writes a string directly into memory, and the child attaches to that same memory segment to read it instantly.
4. Shared Semaphore(SharedSemaphore\
`SharedSemaphorePOSIX, SharedSemaphoreWIN`In this example, you can see how to synchronize two processes to prevent them from crashing or corrupting data.
    - It demonstrates: How to make a child process wait (block) until the parent signals it is safe to proceed. Without this semaphore, the child might try to read data before the parent has finished writing it.
5. Socket Channel(SocketChannel)\
`socketPOSIX, socketWIN`In this example, you can see a Client-Server architecture where one process listens for connections and another connects to it.
    - It demonstrates: Bidirectional communication using TCP (Windows/Linux) or Unix Domain Sockets (Linux). This allows processes to talk to each other even if they are on different computers (TCP).
