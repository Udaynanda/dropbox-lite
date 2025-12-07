#include "core/delta_engine.h"
#include "common/hash.h"
#include <fstream>
#include <algorithm>

namespace dropboxlite {

DeltaEngine::DeltaEngine(MetadataDB& db) : db_(db) {}

DeltaInfo DeltaEngine::computeDelta(const std::string& filepath,
                                   const std::vector<std::string>& server_chunk_hashes) {
    DeltaInfo delta;
    delta.bytes_to_transfer = 0;
    
    // Chunk the local file
    auto local_chunks = chunker_.chunkFile(filepath);
    
    // Convert server hashes to set for fast lookup
    auto server_set = convertToSet(server_chunk_hashes);
    
    // Determine which chunks need to be uploaded
    for (const auto& chunk : local_chunks) {
        if (server_set.find(chunk.hash) == server_set.end()) {
            // Server doesn't have this chunk
            delta.new_chunks.push_back(chunk);
            delta.bytes_to_transfer += chunk.size;
        } else {
            // Server already has this chunk
            delta.existing_chunks.push_back(chunk);
        }
    }
    
    return delta;
}

bool DeltaEngine::applyDelta(const std::string& filepath,
                            const std::vector<ChunkInfo>& chunks,
                            const std::vector<uint8_t>& chunk_data) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        return false;
    }
    
    size_t data_offset = 0;
    for (const auto& chunk : chunks) {
        if (data_offset + chunk.size > chunk_data.size()) {
            return false;
        }
        
        file.write(reinterpret_cast<const char*>(chunk_data.data() + data_offset),
                  chunk.size);
        data_offset += chunk.size;
    }
    
    return true;
}

bool DeltaEngine::areFilesIdentical(const std::string& path1, const std::string& path2) {
    std::string hash1 = Hash::sha256File(path1);
    std::string hash2 = Hash::sha256File(path2);
    return !hash1.empty() && hash1 == hash2;
}

std::set<std::string> DeltaEngine::convertToSet(const std::vector<std::string>& hashes) {
    return std::set<std::string>(hashes.begin(), hashes.end());
}

} // namespace dropboxlite
