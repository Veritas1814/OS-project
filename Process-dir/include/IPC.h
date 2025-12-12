#pragma once

#include "Process.h"
#include "Pipe.h"
#include "SocketChannel.h"
#include "SharedMemoryChannel.h"
#include "Semaphore.h"

namespace ipc {
    using Process       = ::Process;
    using PipeChannel   = ::Pipe;
    using SocketChannel = ::SocketChannel;
    using SharedMemory  = ::SharedMemoryChannel;

}