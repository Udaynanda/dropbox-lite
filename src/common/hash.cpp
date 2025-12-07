#include "common/hash.h"
#include <openssl/sha.h>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace dropboxlite {

std::string Hash::sha256(const std::vector<uint8_t>& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string Hash::sha256(const std::string& data) {
    std::vector<uint8_t> vec(data.begin(), data.end());
    return sha256(vec);
}

std::string Hash::sha256File(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        return "";
    }
    
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    
    constexpr size_t buffer_size = 8192;
    char buffer[buffer_size];
    
    while (file.read(buffer, buffer_size) || file.gcount() > 0) {
        SHA256_Update(&ctx, buffer, file.gcount());
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// Rolling Hash implementation
Hash::RollingHash::RollingHash(size_t window_size)
    : window_size_(window_size), hash_(0), power_(1) {
    for (size_t i = 0; i < window_size_ - 1; i++) {
        power_ = (power_ * kPrime) % kMod;
    }
}

void Hash::RollingHash::reset() {
    hash_ = 0;
}

void Hash::RollingHash::update(uint8_t byte_in, uint8_t byte_out) {
    hash_ = (hash_ + kMod - (byte_out * power_) % kMod) % kMod;
    hash_ = (hash_ * kPrime + byte_in) % kMod;
}

void Hash::RollingHash::append(uint8_t byte) {
    hash_ = (hash_ * kPrime + byte) % kMod;
}

} // namespace dropboxlite
