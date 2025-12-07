#include "server/storage_manager.h"
#include "common/logger.h"
#include "common/hash.h"
#include <filesystem>
#include <fstream>

namespace dropboxlite {

StorageManager::StorageManager(const std::string& storage_root)
    : storage_root_(storage_root) {}

bool StorageManager::initialize() {
    // Create storage root directory
    std::filesystem::create_directories(storage_root_);
    std::filesystem::create_directories(storage_root_ + "/chunks");
    
    LOG_INFO("Storage manager initialized at: " + storage_root_);
    return true;
}

bool StorageManager::storeChunk(const std::string& client_id,
                               const std::string& filepath,
                               int32_t chunk_index,
                               const std::vector<uint8_t>& data,
                               const std::string& hash) {
    // Store chunk in content-addressable storage
    std::string chunk_path = getChunkPath(hash);
    
    // Check if chunk already exists (deduplication)
    if (std::filesystem::exists(chunk_path)) {
        LOG_DEBUG("Chunk already exists: " + hash);
    } else {
        std::ofstream file(chunk_path, std::ios::binary);
        if (!file) {
            LOG_ERROR("Failed to write chunk: " + chunk_path);
            return false;
        }
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
    
    // Update metadata
    auto* db = getClientDB(client_id);
    if (!db) {
        return false;
    }
    
    return db->insertChunk(filepath, chunk_index, hash, 0, data.size());
}

std::vector<uint8_t> StorageManager::getChunk(const std::string& hash) {
    std::string chunk_path = getChunkPath(hash);
    
    std::ifstream file(chunk_path, std::ios::binary);
    if (!file) {
        LOG_ERROR("Chunk not found: " + hash);
        return {};
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    
    return data;
}

bool StorageManager::hasChunk(const std::string& hash) {
    return std::filesystem::exists(getChunkPath(hash));
}

bool StorageManager::finalizeFile(const std::string& client_id,
                                 const std::string& filepath,
                                 int32_t total_chunks) {
    auto* db = getClientDB(client_id);
    if (!db) {
        return false;
    }
    
    // Get all chunks for the file
    auto chunk_hashes = db->getFileChunks(filepath);
    
    if (chunk_hashes.size() != static_cast<size_t>(total_chunks)) {
        LOG_ERROR("Incomplete file upload: expected " + std::to_string(total_chunks) +
                 " chunks, got " + std::to_string(chunk_hashes.size()));
        return false;
    }
    
    // Reconstruct file from chunks
    std::string temp_path = getTempFilePath(client_id, filepath);
    std::filesystem::create_directories(std::filesystem::path(temp_path).parent_path());
    
    std::ofstream file(temp_path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    for (const auto& hash : chunk_hashes) {
        auto chunk_data = getChunk(hash);
        if (chunk_data.empty()) {
            return false;
        }
        file.write(reinterpret_cast<const char*>(chunk_data.data()), chunk_data.size());
    }
    
    file.close();
    
    // Update file metadata
    FileRecord record;
    record.path = filepath;
    record.size = std::filesystem::file_size(temp_path);
    record.modified_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    record.hash = Hash::sha256File(temp_path);
    record.version = 1;
    record.is_directory = false;
    record.deleted = false;
    
    return db->insertOrUpdateFile(record);
}

std::optional<FileRecord> StorageManager::getFileMetadata(const std::string& client_id,
                                                         const std::string& filepath) {
    auto* db = getClientDB(client_id);
    if (!db) {
        return std::nullopt;
    }
    
    return db->getFile(filepath);
}

std::vector<FileRecord> StorageManager::listFiles(const std::string& client_id) {
    auto* db = getClientDB(client_id);
    if (!db) {
        return {};
    }
    
    return db->getAllFiles();
}

bool StorageManager::deleteFile(const std::string& client_id, const std::string& filepath) {
    auto* db = getClientDB(client_id);
    if (!db) {
        return false;
    }
    
    return db->deleteFile(filepath);
}

StorageManager::StorageStats StorageManager::getStats() const {
    StorageStats stats = {0, 0, 0, 0};
    
    // Count chunks
    std::string chunks_dir = storage_root_ + "/chunks";
    for (const auto& entry : std::filesystem::recursive_directory_iterator(chunks_dir)) {
        if (entry.is_regular_file()) {
            stats.total_chunks++;
            stats.total_bytes += entry.file_size();
        }
    }
    
    return stats;
}

std::string StorageManager::getChunkPath(const std::string& hash) {
    // Use first 2 chars as subdirectory for better filesystem performance
    std::string subdir = hash.substr(0, 2);
    std::string path = storage_root_ + "/chunks/" + subdir;
    std::filesystem::create_directories(path);
    return path + "/" + hash;
}

std::string StorageManager::getClientStoragePath(const std::string& client_id) {
    std::string path = storage_root_ + "/clients/" + client_id;
    std::filesystem::create_directories(path);
    return path;
}

std::string StorageManager::getTempFilePath(const std::string& client_id,
                                           const std::string& filepath) {
    return getClientStoragePath(client_id) + "/" + filepath;
}

MetadataDB* StorageManager::getClientDB(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    
    auto it = client_dbs_.find(client_id);
    if (it != client_dbs_.end()) {
        return it->second.get();
    }
    
    // Create new database for client
    std::string db_path = getClientStoragePath(client_id) + "/metadata.db";
    auto db = std::make_unique<MetadataDB>(db_path);
    
    if (!db->initialize()) {
        LOG_ERROR("Failed to initialize database for client: " + client_id);
        return nullptr;
    }
    
    auto* db_ptr = db.get();
    client_dbs_[client_id] = std::move(db);
    
    return db_ptr;
}

} // namespace dropboxlite
