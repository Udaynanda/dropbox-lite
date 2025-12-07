#include "core/conflict_resolver.h"
#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace dropboxlite {

bool ConflictResolver::hasConflict(const ConflictInfo& info) {
    // No conflict if hashes match
    if (info.local_hash == info.remote_hash) {
        return false;
    }
    
    // Conflict if both versions have been modified
    return info.local_version > 0 && info.remote_version > 0;
}

std::string ConflictResolver::resolve(const ConflictInfo& info, ConflictStrategy strategy) {
    switch (strategy) {
        case ConflictStrategy::KEEP_LOCAL:
            return info.path;
            
        case ConflictStrategy::KEEP_REMOTE:
            return info.path;
            
        case ConflictStrategy::KEEP_BOTH: {
            // Keep remote as original, rename local
            return generateConflictName(info.path, "local");
        }
            
        case ConflictStrategy::MANUAL:
        default:
            return "";
    }
}

std::string ConflictResolver::generateConflictName(const std::string& original_path,
                                                   const std::string& client_id) {
    std::filesystem::path path(original_path);
    std::string base = getBaseName(original_path);
    std::string ext = getFileExtension(original_path);
    
    // Generate timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << base << " (conflicted copy " << client_id << " ";
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H-%M-%S");
    ss << ")";
    
    if (!ext.empty()) {
        ss << "." << ext;
    }
    
    return (path.parent_path() / ss.str()).string();
}

ConflictStrategy ConflictResolver::autoResolve(const ConflictInfo& info) {
    // Last-write-wins strategy
    if (info.local_modified_time > info.remote_modified_time) {
        return ConflictStrategy::KEEP_LOCAL;
    } else if (info.remote_modified_time > info.local_modified_time) {
        return ConflictStrategy::KEEP_REMOTE;
    }
    
    // If timestamps are equal, keep both
    return ConflictStrategy::KEEP_BOTH;
}

std::string ConflictResolver::getFileExtension(const std::string& path) {
    std::filesystem::path p(path);
    std::string ext = p.extension().string();
    if (!ext.empty() && ext[0] == '.') {
        ext = ext.substr(1);
    }
    return ext;
}

std::string ConflictResolver::getBaseName(const std::string& path) {
    std::filesystem::path p(path);
    return p.stem().string();
}

} // namespace dropboxlite
