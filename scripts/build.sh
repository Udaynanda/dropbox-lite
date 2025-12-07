#!/bin/bash
set -e

echo "Building Dropbox Lite..."

# Create build directory
mkdir -p build
cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Build complete!"
echo "Binaries:"
echo "  - dropbox_server_app"
echo "  - dropbox_client_app"
