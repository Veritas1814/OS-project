cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME process_project)
project(${PROJECT_NAME} C CXX)

set(CMAKE_CXX_STANDARD 20)

set(WARNINGS_AS_ERRORS OFF)
set(ENABLE_PVS_STUDIO ON)
set(ENABLE_UBSan OFF)
set(ENABLE_ASAN OFF)
set(ENABLE_TSan OFF)
set(ENABLE_MSAN OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/cmake/CompilerWarnings.cmake")
    include(cmake/CompilerWarnings.cmake)
endif()

#=========================================================
# Process library
#=========================================================

file(GLOB PROCESS_SOURCES "Process-dir/src/*.cpp")
# REMOVED: list(FILTER PROCESS_SOURCES EXCLUDE REGEX ".*/SharedMemoryChannel\\.cpp$")
add_library(Process ${PROCESS_SOURCES})

target_include_directories(Process PUBLIC
    Process-dir/include
)

if (WIN32)
    target_compile_definitions(Process PRIVATE OS_WINDOWS)
elseif(APPLE)
    target_compile_definitions(Process PRIVATE OS_MACOS)
elseif(UNIX)
    target_compile_definitions(Process PRIVATE OS_LINUX)
endif()

#=========================================================
# Executables
#=========================================================

add_executable(test_process
    Process-dir/tests/test_process.cpp
)
target_link_libraries(test_process PRIVATE Process)

add_executable(test_pipe
    Process-dir/tests/test_pipe.cpp
)
target_link_libraries(test_pipe PRIVATE Process)

add_executable(child_socket
    Process-dir/tests/child_socket.cpp
)
target_link_libraries(child_socket PRIVATE Process)
add_executable(test_socket
    Process-dir/tests/demo_socket_test.cpp
)
target_link_libraries(test_socket PRIVATE Process)
add_executable(test_shared_memmory
    Process-dir/tests/final_tests_shared_memmory.cpp
)
target_link_libraries(test_shared_memmory PRIVATE Process)

add_executable(test_process_shared
    Process-dir/tests/test_process_shared.cpp
)
target_link_libraries(test_process_shared PRIVATE Process)

add_executable(test_child_shared
    Process-dir/tests/test_child_shared.cpp
)
target_link_libraries(test_child_shared PRIVATE Process)

add_executable(final_tests_shared_symophore
    Process-dir/tests/final_tests_shared_symophore.cpp
)
target_link_libraries(final_tests_shared_symophore PRIVATE Process)

add_executable(full_shared_semaphore_test
    Process-dir/tests/full_shared_semaphore_test.cpp
)
target_link_libraries(full_shared_semaphore_test PRIVATE Process)

# optional root executable
add_executable(${PROJECT_NAME}
    Process-dir/tests/test_process.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE Process)

#=========================================================
# Install 
#=========================================================

install(TARGETS
    ${PROJECT_NAME}
    test_process
    test_pipe
    test_socket
    test_shared_memmory
    test_process_shared
    full_shared_semaphore_test
    test_child_shared
    RUNTIME DESTINATION bin
)

#=========================================================
# Main config include
#=========================================================

set(ALL_TARGETS
    ${PROJECT_NAME}
    test_process
    test_pipe
    test_socket
    test_shared_memmory
    test_process_shared
    full_shared_semaphore_test
    test_child_shared
)

if (EXISTS "${CMAKE_SOURCE_DIR}/cmake/main-config.cmake")
    include(cmake/main-config.cmake)
endif()

#=========================================================
# PVS-Studio integration
#=========================================================

if (ENABLE_PVS_STUDIO)
    find_program(PVS_STUDIO_ANALYZER pvs-studio-analyzer)
    if (PVS_STUDIO_ANALYZER)
        message(STATUS "PVS-Studio found: ${PVS_STUDIO_ANALYZER}")
        add_custom_target(pvs
            COMMAND ${PVS_STUDIO_ANALYZER} analyze -o PVS-Studio.log --file compile_commands.json
            COMMAND plog-converter -a GA:1,2 -t fullhtml PVS-Studio.log -o PVS-report
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running PVS-Studio analysis"
        )
    else()
        message(WARNING "PVS-Studio not found on system")
    endif()
endif()