#include "common/chunker.h"
#include "common/hash.h"
#include "common/metrics.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <random>
#include <iomanip>

using namespace dropboxlite;

// Generate test file with specific content
void generateTestFile(const std::string& path, size_t size_mb, const std::string& type) {
    std::ofstream file(path, std::ios::binary);
    std::vector<uint8_t> buffer(1024 * 1024); // 1MB buffer
    
    std::random_device rd;
    std::mt19937 gen(rd());
    
    if (type == "random") {
        std::uniform_int_distribution<> dis(0, 255);
        for (size_t i = 0; i < size_mb; i++) {
            for (auto& byte : buffer) {
                byte = dis(gen);
            }
            file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
        }
    } else if (type == "text") {
        // Simulate text with repeated patterns
        const std::string pattern = "The quick brown fox jumps over the lazy dog. ";
        for (size_t i = 0; i < size_mb; i++) {
            for (size_t j = 0; j < buffer.size(); j++) {
                buffer[j] = pattern[j % pattern.size()];
            }
            file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
        }
    } else if (type == "zeros") {
        std::fill(buffer.begin(), buffer.end(), 0);
        for (size_t i = 0; i < size_mb; i++) {
            file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
        }
    }
}

void benchmarkChunking(const std::string& file_path, size_t file_size_mb) {
    Chunker chunker;
    
    auto start = std::chrono::high_resolution_clock::now();
    auto chunks = chunker.chunkFile(file_path);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double seconds = duration.count() / 1000.0;
    double throughput_mbs = file_size_mb / seconds;
    
    auto stats = chunker.getLastStats();
    
    std::cout << "  Chunks: " << chunks.size() << "\n";
    std::cout << "  Avg chunk size: " << std::fixed << std::setprecision(1) 
              << (stats.avg_size / 1024.0) << " KB\n";
    std::cout << "  Min chunk size: " << (stats.min_size / 1024.0) << " KB\n";
    std::cout << "  Max chunk size: " << (stats.max_size / 1024.0) << " KB\n";
    std::cout << "  Time: " << std::setprecision(2) << seconds << " seconds\n";
    std::cout << "  Throughput: " << std::setprecision(1) << throughput_mbs << " MB/s\n";
}

void benchmarkHashing(const std::string& file_path, size_t file_size_mb) {
    auto start = std::chrono::high_resolution_clock::now();
    std::string hash = Hash::sha256File(file_path);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double seconds = duration.count() / 1000.0;
    double throughput_mbs = file_size_mb / seconds;
    
    std::cout << "  Hash: " << hash.substr(0, 16) << "...\n";
    std::cout << "  Time: " << std::fixed << std::setprecision(2) << seconds << " seconds\n";
    std::cout << "  Throughput: " << std::setprecision(1) << throughput_mbs << " MB/s\n";
}

int main() {
    std::cout << "=== Dropbox Lite Performance Benchmarks ===\n\n";
    
    const size_t test_size_mb = 100; // 100MB test file
    
    // Test different content types
    std::vector<std::pair<std::string, std::string>> tests = {
        {"random", "Random data"},
        {"text", "Text (repeated patterns)"},
        {"zeros", "Zeros (highly compressible)"}
    };
    
    for (const auto& [type, description] : tests) {
        std::string file_path = "/tmp/bench_" + type + ".dat";
        
        std::cout << "## " << description << " (" << test_size_mb << " MB)\n\n";
        
        std::cout << "Generating test file...\n";
        generateTestFile(file_path, test_size_mb, type);
        
        std::cout << "\n### Chunking Performance\n";
        benchmarkChunking(file_path, test_size_mb);
        
        std::cout << "\n### Hashing Performance (SHA256)\n";
        benchmarkHashing(file_path, test_size_mb);
        
        std::cout << "\n" << std::string(60, '-') << "\n\n";
        
        // Cleanup
        std::remove(file_path.c_str());
    }
    
    return 0;
}
