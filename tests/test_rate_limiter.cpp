#include "common/rate_limiter.h"
#include <gtest/gtest.h>
#include <chrono>

using namespace dropboxlite;

TEST(RateLimiterTest, BasicThrottling) {
    RateLimiter limiter(1024 * 1024); // 1 MB/s
    
    auto start = std::chrono::steady_clock::now();
    
    // Try to send 2 MB (should take ~2 seconds)
    limiter.acquire(1024 * 1024);
    limiter.acquire(1024 * 1024);
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should take at least 1 second (with some tolerance)
    EXPECT_GE(elapsed.count(), 900);
}

TEST(RateLimiterTest, BurstCapacity) {
    RateLimiter limiter(1024, 2048); // 1KB/s with 2KB burst
    
    // Should be able to acquire 2KB immediately
    EXPECT_TRUE(limiter.tryAcquire(2048));
    
    // But not more
    EXPECT_FALSE(limiter.tryAcquire(1));
}

TEST(RateLimiterTest, RateChange) {
    RateLimiter limiter(1024);
    
    EXPECT_EQ(limiter.getRate(), 1024);
    
    limiter.setRate(2048);
    EXPECT_EQ(limiter.getRate(), 2048);
}
