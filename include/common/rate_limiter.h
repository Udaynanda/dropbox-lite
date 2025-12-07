#pragma once

#include <chrono>
#include <mutex>
#include <atomic>

namespace dropboxlite {

// Token bucket rate limiter for bandwidth control
class RateLimiter {
public:
    // bytes_per_second: maximum throughput
    // burst_size: maximum burst capacity
    explicit RateLimiter(size_t bytes_per_second, size_t burst_size = 0);
    
    // Acquire tokens (blocks if necessary)
    void acquire(size_t bytes);
    
    // Try to acquire tokens (non-blocking)
    bool tryAcquire(size_t bytes);
    
    // Set new rate
    void setRate(size_t bytes_per_second);
    
    // Get current rate
    size_t getRate() const { return bytes_per_second_; }
    
    // Get available tokens
    size_t available() const;
    
private:
    void refill();
    
    size_t bytes_per_second_;
    size_t burst_size_;
    std::atomic<double> tokens_;
    std::chrono::steady_clock::time_point last_refill_;
    mutable std::mutex mutex_;
};

} // namespace dropboxlite
