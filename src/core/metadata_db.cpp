#include "core/metadata_db.h"
#include "common/logger.h"
#include <sstream>

namespace dropboxlite {

MetadataDB::MetadataDB(const std::string& db_path)
    : db_(nullptr), db_path_(db_path) {}

MetadataDB::~MetadataDB() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool MetadataDB::initialize() {
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to open database: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }
    
    // Create tables
    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS files (
            path TEXT PRIMARY KEY,
            size INTEGER,
            modified_time INTEGER,
            hash TEXT,
            version INTEGER,
            is_directory INTEGER,
            deleted INTEGER,
            last_sync_time INTEGER
        );
        
        CREATE TABLE IF NOT EXISTS chunks (
            file_path TEXT,
            chunk_index INTEGER,
            hash TEXT,
            offset INTEGER,
            size INTEGER,
            PRIMARY KEY (file_path, chunk_index)
        );
        
        CREATE TABLE IF NOT EXISTS sync_state (
            key TEXT PRIMARY KEY,
            value INTEGER
        );
        
        CREATE INDEX IF NOT EXISTS idx_chunks_hash ON chunks(hash);
        CREATE INDEX IF NOT EXISTS idx_files_modified ON files(modified_time);
    )";
    
    return executeSQL(schema);
}

bool MetadataDB::insertOrUpdateFile(const FileRecord& record) {
    const char* sql = R"(
        INSERT OR REPLACE INTO files 
        (path, size, modified_time, hash, version, is_directory, deleted, last_sync_time)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to prepare statement: " + getErrorMessage());
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, record.path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, record.size);
    sqlite3_bind_int64(stmt, 3, record.modified_time);
    sqlite3_bind_text(stmt, 4, record.hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, record.version);
    sqlite3_bind_int(stmt, 6, record.is_directory ? 1 : 0);
    sqlite3_bind_int(stmt, 7, record.deleted ? 1 : 0);
    sqlite3_bind_int64(stmt, 8, record.last_sync_time);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::optional<FileRecord> MetadataDB::getFile(const std::string& path) {
    const char* sql = "SELECT * FROM files WHERE path = ?";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return std::nullopt;
    }
    
    sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        FileRecord record;
        record.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        record.size = sqlite3_column_int64(stmt, 1);
        record.modified_time = sqlite3_column_int64(stmt, 2);
        record.hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.version = sqlite3_column_int(stmt, 4);
        record.is_directory = sqlite3_column_int(stmt, 5) != 0;
        record.deleted = sqlite3_column_int(stmt, 6) != 0;
        record.last_sync_time = sqlite3_column_int64(stmt, 7);
        
        sqlite3_finalize(stmt);
        return record;
    }
    
    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<FileRecord> MetadataDB::getAllFiles() {
    std::vector<FileRecord> files;
    const char* sql = "SELECT * FROM files WHERE deleted = 0";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return files;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FileRecord record;
        record.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        record.size = sqlite3_column_int64(stmt, 1);
        record.modified_time = sqlite3_column_int64(stmt, 2);
        record.hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.version = sqlite3_column_int(stmt, 4);
        record.is_directory = sqlite3_column_int(stmt, 5) != 0;
        record.deleted = sqlite3_column_int(stmt, 6) != 0;
        record.last_sync_time = sqlite3_column_int64(stmt, 7);
        files.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    return files;
}

bool MetadataDB::insertChunk(const std::string& file_path, int32_t index,
                            const std::string& hash, int64_t offset, int32_t size) {
    const char* sql = R"(
        INSERT OR REPLACE INTO chunks (file_path, chunk_index, hash, offset, size)
        VALUES (?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, file_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, index);
    sqlite3_bind_text(stmt, 3, hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, offset);
    sqlite3_bind_int(stmt, 5, size);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool MetadataDB::executeSQL(const std::string& sql) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL error: " + std::string(err_msg));
        sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

std::string MetadataDB::getErrorMessage() {
    return sqlite3_errmsg(db_);
}

bool MetadataDB::beginTransaction() {
    return executeSQL("BEGIN TRANSACTION");
}

bool MetadataDB::commit() {
    return executeSQL("COMMIT");
}

bool MetadataDB::rollback() {
    return executeSQL("ROLLBACK");
}

int64_t MetadataDB::getLastSyncTime() {
    const char* sql = "SELECT value FROM sync_state WHERE key = 'last_sync_time'";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }
    
    int64_t result = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int64(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return result;
}

bool MetadataDB::updateLastSyncTime(int64_t timestamp) {
    const char* sql = "INSERT OR REPLACE INTO sync_state (key, value) VALUES ('last_sync_time', ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, timestamp);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<FileRecord> MetadataDB::getModifiedSince(int64_t timestamp) {
    std::vector<FileRecord> files;
    const char* sql = "SELECT * FROM files WHERE modified_time > ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return files;
    }
    
    sqlite3_bind_int64(stmt, 1, timestamp);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FileRecord record;
        record.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        record.size = sqlite3_column_int64(stmt, 1);
        record.modified_time = sqlite3_column_int64(stmt, 2);
        record.hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.version = sqlite3_column_int(stmt, 4);
        record.is_directory = sqlite3_column_int(stmt, 5) != 0;
        record.deleted = sqlite3_column_int(stmt, 6) != 0;
        record.last_sync_time = sqlite3_column_int64(stmt, 7);
        files.push_back(record);
    }
    
    sqlite3_finalize(stmt);
    return files;
}

bool MetadataDB::deleteFile(const std::string& path) {
    const char* sql = "UPDATE files SET deleted = 1 WHERE path = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<std::string> MetadataDB::getFileChunks(const std::string& file_path) {
    std::vector<std::string> hashes;
    const char* sql = "SELECT hash FROM chunks WHERE file_path = ? ORDER BY chunk_index";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return hashes;
    }
    
    sqlite3_bind_text(stmt, 1, file_path.c_str(), -1, SQLITE_TRANSIENT);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        hashes.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    
    sqlite3_finalize(stmt);
    return hashes;
}

bool MetadataDB::hasChunk(const std::string& hash) {
    const char* sql = "SELECT 1 FROM chunks WHERE hash = ? LIMIT 1";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_TRANSIENT);
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    
    return exists;
}

// RAII Transaction implementation
MetadataDB::Transaction::Transaction(MetadataDB& db) 
    : db_(db), committed_(false) {
    db_.beginTransaction();
}

MetadataDB::Transaction::~Transaction() {
    if (!committed_) {
        db_.rollback();
    }
}

bool MetadataDB::Transaction::commit() {
    if (db_.commit()) {
        committed_ = true;
        return true;
    }
    return false;
}

void MetadataDB::Transaction::rollback() {
    if (!committed_) {
        db_.rollback();
        committed_ = true; // Prevent double rollback
    }
}

} // namespace dropboxlite
