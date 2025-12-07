#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace dropboxlite {

class Hash {
public:
    // Compute SHA256 hash of data
    static std::string sha256(const std::vector<uint8_t>& data);
    static std::string sha256(const std::string& data);
    static std::string sha256File(const std::string& filepath);
    
    // Rolling hash for efficient chunking (Rabin-Karp)
    class RollingHash {
    public:
        RollingHash(size_t window_size);
        
        void reset();
        uint64_t hash() const { return hash_; }
        void update(uint8_t byte_in, uint8_t byte_out);
        void append(uint8_t byte);
        
    private:
        size_t window_size_;
        uint64_t hash_;
        uint64_t power_;
        static constexpr uint64_t kPrime = 31;
        static constexpr uint64_t kMod = 1e9 + 9;
    };
    
private:
    Hash() = default;
};

} // namespace dropboxlite
