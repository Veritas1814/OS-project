cmake_minimum_required(VERSION 3.15)

#=========================================================
# Project setup
#=========================================================

set(PROJECT_NAME process_project)
project(${PROJECT_NAME} C CXX)

set(CMAKE_CXX_STANDARD 20)

#=========================================================
# Configuration options
#=========================================================

set(WARNINGS_AS_ERRORS OFF)
set(ENABLE_PVS_STUDIO ON)
set(ENABLE_UBSan OFF)
set(ENABLE_ASAN OFF)
set(ENABLE_TSan OFF)
set(ENABLE_MSAN OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Include compiler warning configuration if present
if (EXISTS "${CMAKE_SOURCE_DIR}/cmake/CompilerWarnings.cmake")
    include(cmake/CompilerWarnings.cmake)
endif()

#=========================================================
# Process library
#=========================================================
file(GLOB PROCESS_SOURCES "Process-dir/src/*.cpp")

add_library(Process ${PROCESS_SOURCES})

target_include_directories(Process PUBLIC
        Process-dir/include
)

#=========================================================
# Main executable (using test_process.cpp)
#=========================================================

add_executable(${PROJECT_NAME}
        Process-dir/tests/test_process.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE Process)

#=========================================================
# Cross-platform specifics
#=========================================================

if (WIN32)
    target_compile_definitions(Process PRIVATE OS_WINDOWS)
elseif(APPLE)
    target_compile_definitions(Process PRIVATE OS_MACOS)
elseif(UNIX)
    target_compile_definitions(Process PRIVATE OS_LINUX)
endif()

#=========================================================
# Install rule
#=========================================================

INSTALL(PROGRAMS
        $<TARGET_FILE:${PROJECT_NAME}>
        DESTINATION bin
)

#=========================================================
# Fixed configuration
#=========================================================

set(ALL_TARGETS ${PROJECT_NAME})
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
