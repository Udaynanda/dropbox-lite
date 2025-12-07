#include "common/chunker.h"
#include "common/hash.h"
#include <iostream>
#include <fstream>
#include <set>
#include <iomanip>

using namespace dropboxlite;

void createSimilarFiles(const std::string& base_path, int count) {
    // Create base content
    std::string base_content;
    for (int i = 0; i < 10000; i++) {
        base_content += "This is line " + std::to_string(i) + " of the document.\n";
    }
    
    for (int i = 0; i < count; i++) {
        std::string file_path = base_path + "_" + std::to_string(i) + ".txt";
        std::ofstream file(file_path);
        
        // Write base content
        file << base_content;
        
        // Add unique content (10% different)
        for (int j = 0; j < 1000; j++) {
            file << "Unique content for file " << i << " line " << j << "\n";
        }
    }
}

int main() {
    std::cout << "=== Deduplication Effectiveness Test ===\n\n";
    
    const int num_files = 10;
    const std::string base_path = "/tmp/dedup_test";
    
    std::cout << "Creating " << num_files << " similar files (90% overlap)...\n";
    createSimilarFiles(base_path, num_files);
    
    Chunker chunker;
    std::set<std::string> unique_chunks;
    size_t total_chunks = 0;
    size_t total_bytes = 0;
    size_t unique_bytes = 0;
    
    // Process each file
    for (int i = 0; i < num_files; i++) {
        std::string file_path = base_path + "_" + std::to_string(i) + ".txt";
        auto chunks = chunker.chunkFile(file_path);
        
        for (const auto& chunk : chunks) {
            total_chunks++;
            total_bytes += chunk.size;
            
            if (unique_chunks.insert(chunk.hash).second) {
                // New unique chunk
                unique_bytes += chunk.size;
            }
        }
        
        std::remove(file_path.c_str());
    }
    
    double dedup_ratio = 100.0 * (1.0 - (double)unique_bytes / total_bytes);
    double storage_savings = 100.0 * (total_bytes - unique_bytes) / total_bytes;
    
    std::cout << "\n### Results\n";
    std::cout << "Total chunks: " << total_chunks << "\n";
    std::cout << "Unique chunks: " << unique_chunks.size() << "\n";
    std::cout << "Total size: " << (total_bytes / 1024.0 / 1024.0) << " MB\n";
    std::cout << "Unique size: " << (unique_bytes / 1024.0 / 1024.0) << " MB\n";
    std::cout << "Deduplication ratio: " << std::fixed << std::setprecision(1) 
              << dedup_ratio << "%\n";
    std::cout << "Storage savings: " << storage_savings << "%\n";
    
    return 0;
}
