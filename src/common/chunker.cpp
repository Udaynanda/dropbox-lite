#include "common/chunker.h"
#include "common/hash.h"
#include <fstream>
#include <algorithm>

namespace dropboxlite {

std::vector<ChunkInfo> Chunker::chunkFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        return {};
    }
    
    // Read entire file
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(file_size);
    file.read(reinterpret_cast<char*>(data.data()), file_size);
    
    return chunkData(data);
}

std::vector<ChunkInfo> Chunker::chunkData(const std::vector<uint8_t>& data) {
    std::vector<ChunkInfo> chunks;
    
    if (data.empty()) {
        last_stats_ = {0, 0, 0, 0.0};
        return chunks;
    }
    
    // FastCDC-inspired chunking with gear hash
    Hash::RollingHash rolling_hash(48); // 48-byte window
    size_t chunk_start = 0;
    size_t min_size = 0;
    size_t max_size = 0;
    size_t total_size = 0;
    
    // Normalize cut point (FastCDC optimization)
    size_t normalized_chunk_size = kMinChunkSize + (kAvgChunkSize - kMinChunkSize) / 2;
    
    for (size_t i = 0; i < data.size(); i++) {
        rolling_hash.append(data[i]);
        size_t chunk_size = i + 1 - chunk_start;
        
        // Check for chunk boundary conditions
        bool is_boundary = false;
        bool max_size_reached = chunk_size >= kMaxChunkSize;
        bool is_last_byte = (i == data.size() - 1);
        
        // Use different masks for different regions (FastCDC)
        if (chunk_size >= kMinChunkSize && chunk_size < normalized_chunk_size) {
            // Smaller mask for early region (more likely to cut)
            is_boundary = (rolling_hash.hash() & (kMask >> 1)) == 0;
        } else if (chunk_size >= normalized_chunk_size) {
            // Normal mask for main region
            is_boundary = (rolling_hash.hash() & kMask) == 0;
        }
        
        if (is_boundary || max_size_reached || is_last_byte) {
            // Create chunk
            std::vector<uint8_t> chunk_data(
                data.begin() + chunk_start,
                data.begin() + i + 1
            );
            
            ChunkInfo chunk;
            chunk.offset = chunk_start;
            chunk.size = chunk_data.size();
            chunk.hash = Hash::sha256(chunk_data);
            
            chunks.push_back(chunk);
            
            // Update stats
            if (min_size == 0 || chunk.size < min_size) min_size = chunk.size;
            if (chunk.size > max_size) max_size = chunk.size;
            total_size += chunk.size;
            
            chunk_start = i + 1;
            rolling_hash.reset();
        }
    }
    
    // Store statistics
    last_stats_ = {
        chunks.size(),
        min_size,
        max_size,
        chunks.empty() ? 0.0 : static_cast<double>(total_size) / chunks.size()
    };
    
    return chunks;
}

bool Chunker::isChunkBoundary(uint64_t hash, size_t current_size) const {
    // FastCDC: Use bit masking instead of modulo for better performance
    if (current_size < kMinChunkSize) {
        return false;
    }
    
    // Adaptive masking based on chunk size
    if (current_size < kAvgChunkSize) {
        return (hash & (kMask >> 1)) == 0; // More aggressive cutting
    }
    
    return (hash & kMask) == 0;
}

} // namespace dropboxlite
