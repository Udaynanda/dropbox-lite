#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace dropboxlite {

struct ChunkInfo {
    size_t offset;
    size_t size;
    std::string hash;
};

class Chunker {
public:
    // Content-defined chunking using rolling hash (FastCDC-inspired)
    static constexpr size_t kMinChunkSize = 4 * 1024;      // 4KB
    static constexpr size_t kAvgChunkSize = 64 * 1024;     // 64KB
    static constexpr size_t kMaxChunkSize = 1024 * 1024;   // 1MB
    static constexpr uint64_t kMaskBits = 16;              // For ~64KB avg
    static constexpr uint64_t kMask = (1ULL << kMaskBits) - 1;
    
    Chunker() = default;
    
    // Split file into variable-size chunks based on content
    // Returns empty vector on error
    std::vector<ChunkInfo> chunkFile(const std::string& filepath);
    
    // Split data buffer into chunks
    // Uses FastCDC algorithm for better chunk distribution
    std::vector<ChunkInfo> chunkData(const std::vector<uint8_t>& data);
    
    // Get chunking statistics
    struct ChunkStats {
        size_t total_chunks;
        size_t min_size;
        size_t max_size;
        double avg_size;
    };
    ChunkStats getLastStats() const { return last_stats_; }
    
private:
    bool isChunkBoundary(uint64_t hash, size_t current_size) const;
    mutable ChunkStats last_stats_;
};

} // namespace dropboxlite
