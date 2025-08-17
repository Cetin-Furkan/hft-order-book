#!/bin/bash

# =============================================================================
# Build and Run Script for the Hermes Project
# =============================================================================
# This script automates the entire build and execution process.
#
# 1. It configures the project with CMake for a Release build using icx.
# 2. It compiles the project using make.
# 3. It runs the final executable.
#
# It also includes error checking: if any step fails, the script will exit
# immediately to prevent further issues.
# =============================================================================

# Exit immediately if a command exits with a non-zero status.
set -e

# --- Configuration ---
BUILD_DIR="build"
BUILD_TYPE="Release"
COMPILER="icx"
TARGET_EXEC="hermes"

# --- Setup Environment ---
# Source the Intel oneAPI environment script to find the icx compiler.
# We add '|| true' to prevent the script from exiting if setvars.sh
# has already been run and returns a non-zero status code.
echo "==> Sourcing Intel oneAPI environment..."
source /opt/intel/oneapi/setvars.sh || true

# --- Build Process ---
# Create the build directory if it doesn't exist.
echo "==> Creating build directory..."
mkdir -p ${BUILD_DIR}

# Navigate into the build directory.
cd ${BUILD_DIR}

# Run CMake to configure the project and generate the Makefile.
echo "==> Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_C_COMPILER=${COMPILER}

# Run make to compile the code. The '-j' flag tells make to use all
# available CPU cores for a faster parallel build.
echo "==> Compiling with make..."
make -j

# --- Execution ---
# Run the final executable.
echo "==> Running target: ${TARGET_EXEC}"
./${TARGET_EXEC}
