#pragma once

#include "common/chunker.h"
#include "core/metadata_db.h"
#include <vector>
#include <string>
#include <set>

namespace dropboxlite {

struct DeltaInfo {
    std::vector<ChunkInfo> new_chunks;      // Chunks to upload
    std::vector<ChunkInfo> existing_chunks; // Chunks already on server
    size_t bytes_to_transfer;
};

class DeltaEngine {
public:
    explicit DeltaEngine(MetadataDB& db);
    
    // Compute delta between local file and server version
    DeltaInfo computeDelta(const std::string& filepath,
                          const std::vector<std::string>& server_chunk_hashes);
    
    // Apply delta to reconstruct file
    bool applyDelta(const std::string& filepath,
                   const std::vector<ChunkInfo>& chunks,
                   const std::vector<uint8_t>& chunk_data);
    
    // Check if files are identical based on hash
    bool areFilesIdentical(const std::string& path1, const std::string& path2);
    
private:
    MetadataDB& db_;
    Chunker chunker_;
    
    std::set<std::string> convertToSet(const std::vector<std::string>& hashes);
};

} // namespace dropboxlite
