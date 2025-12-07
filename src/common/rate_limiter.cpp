#include "common/rate_limiter.h"
#include <thread>
#include <algorithm>

namespace dropboxlite {

RateLimiter::RateLimiter(size_t bytes_per_second, size_t burst_size)
    : bytes_per_second_(bytes_per_second),
      burst_size_(burst_size > 0 ? burst_size : bytes_per_second),
      tokens_(burst_size_),
      last_refill_(std::chrono::steady_clock::now()) {}

void RateLimiter::acquire(size_t bytes) {
    while (!tryAcquire(bytes)) {
        // Sleep for the time needed to accumulate required tokens
        double tokens_needed = bytes - available();
        double seconds_to_wait = tokens_needed / bytes_per_second_;
        
        auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::duration<double>(seconds_to_wait)
        );
        
        std::this_thread::sleep_for(std::max(wait_time, std::chrono::milliseconds(1)));
    }
}

bool RateLimiter::tryAcquire(size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    refill();
    
    double current_tokens = tokens_.load();
    if (current_tokens >= bytes) {
        tokens_ = current_tokens - bytes;
        return true;
    }
    
    return false;
}

void RateLimiter::setRate(size_t bytes_per_second) {
    std::lock_guard<std::mutex> lock(mutex_);
    bytes_per_second_ = bytes_per_second;
    burst_size_ = bytes_per_second; // Update burst size proportionally
}

size_t RateLimiter::available() const {
    return static_cast<size_t>(std::max(0.0, tokens_.load()));
}

void RateLimiter::refill() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
        now - last_refill_
    ).count();
    
    double tokens_to_add = elapsed * bytes_per_second_;
    double current_tokens = tokens_.load();
    
    tokens_ = std::min(current_tokens + tokens_to_add, static_cast<double>(burst_size_));
    last_refill_ = now;
}

} // namespace dropboxlite
