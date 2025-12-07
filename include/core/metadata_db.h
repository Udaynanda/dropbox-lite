#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <sqlite3.h>

namespace dropboxlite {

struct FileRecord {
    std::string path;
    int64_t size;
    int64_t modified_time;
    std::string hash;
    int32_t version;
    bool is_directory;
    bool deleted;
    int64_t last_sync_time;
};

class MetadataDB {
public:
    explicit MetadataDB(const std::string& db_path);
    ~MetadataDB();
    
    // Initialize database schema
    bool initialize();
    
    // File operations
    bool insertOrUpdateFile(const FileRecord& record);
    std::optional<FileRecord> getFile(const std::string& path);
    std::vector<FileRecord> getAllFiles();
    std::vector<FileRecord> getModifiedSince(int64_t timestamp);
    bool deleteFile(const std::string& path);
    
    // Chunk tracking for deduplication
    bool insertChunk(const std::string& file_path, int32_t index, 
                     const std::string& hash, int64_t offset, int32_t size);
    std::vector<std::string> getFileChunks(const std::string& file_path);
    bool hasChunk(const std::string& hash);
    
    // Sync state
    bool updateLastSyncTime(int64_t timestamp);
    int64_t getLastSyncTime();
    
    // Transaction support with RAII
    class Transaction {
    public:
        explicit Transaction(MetadataDB& db);
        ~Transaction();
        
        bool commit();
        void rollback();
        
        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;
        
    private:
        MetadataDB& db_;
        bool committed_;
    };
    
    bool beginTransaction();
    bool commit();
    bool rollback();
    
private:
    sqlite3* db_;
    std::string db_path_;
    
    bool executeSQL(const std::string& sql);
    std::string getErrorMessage();
};

} // namespace dropboxlite
