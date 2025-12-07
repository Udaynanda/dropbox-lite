#include "common/chunker.h"
#include "common/hash.h"
#include <iostream>
#include <fstream>
#include <set>
#include <iomanip>

using namespace dropboxlite;

void createFile(const std::string& path, size_t size_kb) {
    std::ofstream file(path);
    for (size_t i = 0; i < size_kb; i++) {
        for (int j = 0; j < 1024; j++) {
            file << (char)('A' + (i + j) % 26);
        }
    }
}

void modifyFile(const std::string& path, double modify_percent) {
    std::ifstream in(path, std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
    in.close();
    
    // Modify specified percentage
    size_t bytes_to_modify = data.size() * modify_percent / 100.0;
    size_t start_pos = data.size() / 2; // Modify in the middle
    
    for (size_t i = 0; i < bytes_to_modify && (start_pos + i) < data.size(); i++) {
        data[start_pos + i] = 'X';
    }
    
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<char*>(data.data()), data.size());
}

int main() {
    std::cout << "=== Delta Sync Efficiency Test ===\n\n";
    
    const std::string original_path = "/tmp/delta_original.txt";
    const std::string modified_path = "/tmp/delta_modified.txt";
    const size_t file_size_kb = 10240; // 10 MB
    
    std::cout << "Creating " << file_size_kb / 1024 << " MB test file...\n";
    createFile(original_path, file_size_kb);
    
    std::vector<double> modify_percentages = {0.1, 1.0, 5.0, 10.0, 25.0};
    
    Chunker chunker;
    
    for (double modify_pct : modify_percentages) {
        std::cout << "\n## Modifying " << modify_pct << "% of file\n";
        
        // Create modified version
        std::filesystem::copy_file(original_path, modified_path, 
                                   std::filesystem::copy_options::overwrite_existing);
        modifyFile(modified_path, modify_pct);
        
        // Chunk both versions
        auto original_chunks = chunker.chunkFile(original_path);
        auto modified_chunks = chunker.chunkFile(modified_path);
        
        // Build hash set of original chunks
        std::set<std::string> original_hashes;
        size_t original_bytes = 0;
        for (const auto& chunk : original_chunks) {
            original_hashes.insert(chunk.hash);
            original_bytes += chunk.size;
        }
        
        // Find new chunks in modified version
        size_t new_chunks = 0;
        size_t new_bytes = 0;
        for (const auto& chunk : modified_chunks) {
            if (original_hashes.find(chunk.hash) == original_hashes.end()) {
                new_chunks++;
                new_bytes += chunk.size;
            }
        }
        
        double bandwidth_reduction = 100.0 * (1.0 - (double)new_bytes / original_bytes);
        
        std::cout << "  Original chunks: " << original_chunks.size() << "\n";
        std::cout << "  Modified chunks: " << modified_chunks.size() << "\n";
        std::cout << "  New chunks: " << new_chunks << "\n";
        std::cout << "  Original size: " << (original_bytes / 1024.0 / 1024.0) << " MB\n";
        std::cout << "  Bytes to transfer: " << (new_bytes / 1024.0 / 1024.0) << " MB\n";
        std::cout << "  Bandwidth reduction: " << std::fixed << std::setprecision(1) 
                  << bandwidth_reduction << "%\n";
        
        std::remove(modified_path.c_str());
    }
    
    std::remove(original_path.c_str());
    
    return 0;
}
