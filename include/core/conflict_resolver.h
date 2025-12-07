#pragma once

#include <string>
#include <vector>

namespace dropboxlite {

enum class ConflictStrategy {
    KEEP_LOCAL,
    KEEP_REMOTE,
    KEEP_BOTH,
    MANUAL
};

struct ConflictInfo {
    std::string path;
    std::string local_hash;
    std::string remote_hash;
    int64_t local_modified_time;
    int64_t remote_modified_time;
    int32_t local_version;
    int32_t remote_version;
};

class ConflictResolver {
public:
    ConflictResolver() = default;
    
    // Detect if there's a conflict
    bool hasConflict(const ConflictInfo& info);
    
    // Resolve conflict based on strategy
    std::string resolve(const ConflictInfo& info, ConflictStrategy strategy);
    
    // Generate conflict copy filename
    std::string generateConflictName(const std::string& original_path,
                                     const std::string& client_id);
    
    // Auto-resolve based on timestamps (last-write-wins)
    ConflictStrategy autoResolve(const ConflictInfo& info);
    
private:
    std::string getFileExtension(const std::string& path);
    std::string getBaseName(const std::string& path);
};

} // namespace dropboxlite
