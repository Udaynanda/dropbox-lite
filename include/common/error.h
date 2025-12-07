#pragma once

#include <string>
#include <system_error>

namespace dropboxlite {

// Custom error codes
enum class ErrorCode {
    Success = 0,
    FileNotFound,
    PermissionDenied,
    NetworkError,
    DatabaseError,
    HashMismatch,
    ChunkingFailed,
    CompressionFailed,
    ConflictDetected,
    InvalidArgument,
    OutOfSpace,
    Timeout,
    Unknown
};

// Error category for dropbox-lite errors
class DropboxErrorCategory : public std::error_category {
public:
    const char* name() const noexcept override {
        return "dropbox-lite";
    }
    
    std::string message(int ev) const override {
        switch (static_cast<ErrorCode>(ev)) {
            case ErrorCode::Success: return "Success";
            case ErrorCode::FileNotFound: return "File not found";
            case ErrorCode::PermissionDenied: return "Permission denied";
            case ErrorCode::NetworkError: return "Network error";
            case ErrorCode::DatabaseError: return "Database error";
            case ErrorCode::HashMismatch: return "Hash mismatch - data corruption detected";
            case ErrorCode::ChunkingFailed: return "Chunking operation failed";
            case ErrorCode::CompressionFailed: return "Compression/decompression failed";
            case ErrorCode::ConflictDetected: return "File conflict detected";
            case ErrorCode::InvalidArgument: return "Invalid argument";
            case ErrorCode::OutOfSpace: return "Out of disk space";
            case ErrorCode::Timeout: return "Operation timed out";
            default: return "Unknown error";
        }
    }
};

inline const DropboxErrorCategory& dropbox_category() {
    static DropboxErrorCategory category;
    return category;
}

inline std::error_code make_error_code(ErrorCode e) {
    return {static_cast<int>(e), dropbox_category()};
}

// Result type for operations that can fail
template<typename T>
class Result {
public:
    Result(T value) : value_(std::move(value)), error_(ErrorCode::Success) {}
    Result(ErrorCode error) : error_(error) {}
    Result(ErrorCode error, std::string message) 
        : error_(error), error_message_(std::move(message)) {}
    
    bool ok() const { return error_ == ErrorCode::Success; }
    explicit operator bool() const { return ok(); }
    
    const T& value() const { return value_; }
    T& value() { return value_; }
    
    ErrorCode error() const { return error_; }
    const std::string& errorMessage() const { return error_message_; }
    
    // Unwrap value or throw
    T valueOrThrow() const {
        if (!ok()) {
            throw std::runtime_error(error_message_.empty() 
                ? dropbox_category().message(static_cast<int>(error_))
                : error_message_);
        }
        return value_;
    }
    
private:
    T value_;
    ErrorCode error_;
    std::string error_message_;
};

// Specialization for void
template<>
class Result<void> {
public:
    Result() : error_(ErrorCode::Success) {}
    Result(ErrorCode error) : error_(error) {}
    Result(ErrorCode error, std::string message) 
        : error_(error), error_message_(std::move(message)) {}
    
    bool ok() const { return error_ == ErrorCode::Success; }
    explicit operator bool() const { return ok(); }
    
    ErrorCode error() const { return error_; }
    const std::string& errorMessage() const { return error_message_; }
    
private:
    ErrorCode error_;
    std::string error_message_;
};

} // namespace dropboxlite

// Enable std::error_code integration
namespace std {
    template<>
    struct is_error_code_enum<dropboxlite::ErrorCode> : true_type {};
}
