# =============================================================================
# CMakeLists.txt for the Hermes Project
# =============================================================================
# This file configures the project for CMake, which will then generate a
# native build system (e.g., Makefiles on Linux).
#
# It is designed for high-performance C development using the Intel C++
# Compiler (icx) and the C23 standard.
# =============================================================================

# --- Project Definition ---
cmake_minimum_required(VERSION 3.16)
project(Hermes C)

# --- Compiler Configuration ---
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# --- Source File Definitions ---
# Define the source files for the main 'hermes' application.
set(HERMES_SOURCES
    src/main.c
    src/network.c
    src/orderbook.c
    src/spsc_queue.c
    src/sequencer.c
)

# Define the source files for our test packet sender.
set(TESTER_SOURCES
    src/send_test_packet.c
)

# --- Executable Creation ---
# Create the two separate executable targets.
add_executable(hermes ${HERMES_SOURCES})
add_executable(send_test_packet ${TESTER_SOURCES})

# --- Target Configuration for 'hermes' ---
target_include_directories(hermes PRIVATE include)
target_compile_options(hermes PRIVATE -Wall -Wextra)
target_compile_definitions(hermes PRIVATE _DEFAULT_SOURCE $<$<CONFIG:Release>:NDEBUG>)
target_link_libraries(hermes PRIVATE pthread m)

# --- Target Configuration for 'send_test_packet' ---
# The tester also needs to know where to find the header files.
target_include_directories(send_test_packet PRIVATE include)
target_link_libraries(send_test_packet PRIVATE pthread m)


# --- Target Configuration for 'order_entry' ---
add_executable(order_entry src/order_entry.c)
target_include_directories(order_entry PRIVATE include)
target_link_libraries(order_entry PRIVATE m)


# --- Build Type Specific Flags ---
# These are global settings for the build types.
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -march=native -flto")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g")

# --- User-Friendly Output ---
message(STATUS "Compiler: ${CMAKE_C_COMPILER}")
