#!/bin/bash

# Stop on first error
set -e

# Change to the script's directory. This makes the script runnable from anywhere.
cd "$(dirname "$0")"

echo "========================================="
echo "  BUILDING AND TESTING PROJECT (LINUX)   "
echo "========================================="
echo

# 1. Forcing a clean build
echo "--- FORCE CLEAN: Deleting old build directory ---"
rm -rf build
mkdir build
cd build

# 2. Configuring with CMake
echo "--- Running CMake..."
cmake ..

# 3. Compiling with Make
echo "--- Compiling project..."
make -j$(nproc)

echo
echo "========================================="
echo "          RUNNING TESTS                  "
echo "========================================="
echo

# 4. Setting up test environment for HEADLESS execution
echo "--- Configuring OpenAL for headless testing (no pulseaudio needed)..."
cat > alsoft.conf << EOF
[general]
drivers = wave

[wave]
file = /dev/null
EOF

# 5. Running tests with ctest via xvfb
echo "--- Executing tests via ctest and xvfb-run..."
xvfb-run -a ctest --verbose

echo
echo "========================================="
echo "       BUILD AND TEST COMPLETE         "
echo "========================================="
echo
