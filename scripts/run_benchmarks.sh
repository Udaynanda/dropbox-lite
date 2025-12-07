#!/bin/bash
set -e

echo "Building project..."
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "=========================================="
echo "Running Performance Benchmarks"
echo "=========================================="
echo ""

./benchmarks/bench_chunking > ../BENCHMARK_RESULTS.txt 2>&1
./benchmarks/bench_dedup >> ../BENCHMARK_RESULTS.txt 2>&1
./benchmarks/bench_delta_sync >> ../BENCHMARK_RESULTS.txt 2>&1

cd ..

echo "Results saved to BENCHMARK_RESULTS.txt"
echo ""
cat BENCHMARK_RESULTS.txt
