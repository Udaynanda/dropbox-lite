#include "common/hash.h"
#include <gtest/gtest.h>

using namespace dropboxlite;

TEST(HashTest, SHA256Basic) {
    std::string data = "Hello, World!";
    std::string hash = Hash::sha256(data);
    
    EXPECT_EQ(hash.length(), 64); // SHA256 produces 64 hex chars
    EXPECT_FALSE(hash.empty());
}

TEST(HashTest, SHA256Consistency) {
    std::string data = "test data";
    std::string hash1 = Hash::sha256(data);
    std::string hash2 = Hash::sha256(data);
    
    EXPECT_EQ(hash1, hash2);
}

TEST(HashTest, RollingHash) {
    Hash::RollingHash rh(10);
    
    rh.append('a');
    uint64_t hash1 = rh.hash();
    
    rh.append('b');
    uint64_t hash2 = rh.hash();
    
    EXPECT_NE(hash1, hash2);
}
