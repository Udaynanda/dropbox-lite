#pragma once

#include "core/metadata_db.h"
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

namespace dropboxlite {

class StorageManager {
public:
    explicit StorageManager(const std::string& storage_root);
    
    // Initialize storage
    bool initialize();
    
    // Store file chunk
    bool storeChunk(const std::string& client_id,
                   const std::string& filepath,
                   int32_t chunk_index,
                   const std::vector<uint8_t>& data,
                   const std::string& hash);
    
    // Retrieve file chunk
    std::vector<uint8_t> getChunk(const std::string& hash);
    
    // Check if chunk exists (deduplication)
    bool hasChunk(const std::string& hash);
    
    // Finalize file after all chunks uploaded
    bool finalizeFile(const std::string& client_id,
                     const std::string& filepath,
                     int32_t total_chunks);
    
    // Get file metadata
    std::optional<FileRecord> getFileMetadata(const std::string& client_id,
                                             const std::string& filepath);
    
    // List all files for client
    std::vector<FileRecord> listFiles(const std::string& client_id);
    
    // Delete file
    bool deleteFile(const std::string& client_id, const std::string& filepath);
    
    // Get storage statistics
    struct StorageStats {
        size_t total_files;
        size_t total_chunks;
        size_t total_bytes;
        size_t deduplicated_bytes;
    };
    StorageStats getStats() const;
    
private:
    std::string storage_root_;
    std::unordered_map<std::string, std::unique_ptr<MetadataDB>> client_dbs_;
    mutable std::mutex db_mutex_;
    
    std::string getChunkPath(const std::string& hash);
    std::string getClientStoragePath(const std::string& client_id);
    std::string getTempFilePath(const std::string& client_id,
                               const std::string& filepath);
    
    MetadataDB* getClientDB(const std::string& client_id);
};

} // namespace dropboxlite
