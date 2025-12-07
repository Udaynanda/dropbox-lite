#pragma once

#include <vector>
#include <cstdint>

namespace dropboxlite {

class Compression {
public:
    // Compress data using zlib
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& data);
    
    // Decompress data
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& data);
    
    // Check if compression would be beneficial
    static bool shouldCompress(size_t data_size);
    
private:
    static constexpr size_t kMinCompressionSize = 1024; // 1KB
};

} // namespace dropboxlite
