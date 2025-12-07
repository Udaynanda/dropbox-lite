#include "common/compression.h"
#include <zlib.h>
#include <stdexcept>

namespace dropboxlite {

std::vector<uint8_t> Compression::compress(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return {};
    }
    
    uLongf compressed_size = compressBound(data.size());
    std::vector<uint8_t> compressed(compressed_size);
    
    int result = ::compress(
        compressed.data(),
        &compressed_size,
        data.data(),
        data.size()
    );
    
    if (result != Z_OK) {
        throw std::runtime_error("Compression failed");
    }
    
    compressed.resize(compressed_size);
    return compressed;
}

std::vector<uint8_t> Compression::decompress(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return {};
    }
    
    // Start with 4x the compressed size
    uLongf decompressed_size = data.size() * 4;
    std::vector<uint8_t> decompressed(decompressed_size);
    
    int result = Z_BUF_ERROR;
    while (result == Z_BUF_ERROR) {
        result = uncompress(
            decompressed.data(),
            &decompressed_size,
            data.data(),
            data.size()
        );
        
        if (result == Z_BUF_ERROR) {
            decompressed_size *= 2;
            decompressed.resize(decompressed_size);
        }
    }
    
    if (result != Z_OK) {
        throw std::runtime_error("Decompression failed");
    }
    
    decompressed.resize(decompressed_size);
    return decompressed;
}

bool Compression::shouldCompress(size_t data_size) {
    return data_size >= kMinCompressionSize;
}

} // namespace dropboxlite
