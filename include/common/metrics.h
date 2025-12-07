#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <unordered_map>
#include <mutex>

namespace dropboxlite {

// Performance metrics collection
class Metrics {
public:
    static Metrics& instance();
    
    // Counter operations
    void incrementCounter(const std::string& name, int64_t value = 1);
    int64_t getCounter(const std::string& name) const;
    
    // Gauge operations (current value)
    void setGauge(const std::string& name, int64_t value);
    int64_t getGauge(const std::string& name) const;
    
    // Histogram for latency tracking
    void recordLatency(const std::string& name, std::chrono::microseconds duration);
    
    // Throughput tracking
    void recordBytes(const std::string& name, size_t bytes);
    double getBytesPerSecond(const std::string& name) const;
    
    // Get all metrics as string
    std::string toString() const;
    
    // Reset all metrics
    void reset();
    
private:
    Metrics() = default;
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::atomic<int64_t>> counters_;
    std::unordered_map<std::string, std::atomic<int64_t>> gauges_;
    
    struct LatencyStats {
        std::atomic<int64_t> count{0};
        std::atomic<int64_t> sum_us{0};
        std::atomic<int64_t> min_us{INT64_MAX};
        std::atomic<int64_t> max_us{0};
    };
    std::unordered_map<std::string, LatencyStats> latencies_;
    
    struct ThroughputStats {
        std::atomic<size_t> total_bytes{0};
        std::chrono::steady_clock::time_point start_time;
    };
    std::unordered_map<std::string, ThroughputStats> throughput_;
};

// RAII timer for automatic latency recording
class ScopedTimer {
public:
    explicit ScopedTimer(const std::string& metric_name)
        : metric_name_(metric_name),
          start_(std::chrono::steady_clock::now()) {}
    
    ~ScopedTimer() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
        Metrics::instance().recordLatency(metric_name_, duration);
    }
    
private:
    std::string metric_name_;
    std::chrono::steady_clock::time_point start_;
};

#define MEASURE_LATENCY(name) dropboxlite::ScopedTimer _timer_##__LINE__(name)

} // namespace dropboxlite
