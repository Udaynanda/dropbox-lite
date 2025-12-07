#include "common/chunker.h"
#include <gtest/gtest.h>
#include <fstream>

using namespace dropboxlite;

TEST(ChunkerTest, ChunkData) {
    std::vector<uint8_t> data(100000, 'A'); // 100KB of 'A'
    
    Chunker chunker;
    auto chunks = chunker.chunkData(data);
    
    EXPECT_GT(chunks.size(), 0);
    
    // Verify total size
    size_t total_size = 0;
    for (const auto& chunk : chunks) {
        total_size += chunk.size;
    }
    EXPECT_EQ(total_size, data.size());
}

TEST(ChunkerTest, EmptyData) {
    std::vector<uint8_t> data;
    
    Chunker chunker;
    auto chunks = chunker.chunkData(data);
    
    EXPECT_EQ(chunks.size(), 0);
}
