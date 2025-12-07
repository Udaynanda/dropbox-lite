#include "common/metrics.h"
#include <sstream>
#include <iomanip>

namespace dropboxlite {

Metrics& Metrics::instance() {
    static Metrics metrics;
    return metrics;
}

void Metrics::incrementCounter(const std::string& name, int64_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_[name] += value;
}

int64_t Metrics::getCounter(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = counters_.find(name);
    return it != counters_.end() ? it->second.load() : 0;
}

void Metrics::setGauge(const std::string& name, int64_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    gauges_[name] = value;
}

int64_t Metrics::getGauge(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = gauges_.find(name);
    return it != gauges_.end() ? it->second.load() : 0;
}

void Metrics::recordLatency(const std::string& name, std::chrono::microseconds duration) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& stats = latencies_[name];
    
    int64_t us = duration.count();
    stats.count++;
    stats.sum_us += us;
    
    // Update min
    int64_t current_min = stats.min_us.load();
    while (us < current_min && !stats.min_us.compare_exchange_weak(current_min, us));
    
    // Update max
    int64_t current_max = stats.max_us.load();
    while (us > current_max && !stats.max_us.compare_exchange_weak(current_max, us));
}

void Metrics::recordBytes(const std::string& name, size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& stats = throughput_[name];
    
    if (stats.total_bytes == 0) {
        stats.start_time = std::chrono::steady_clock::now();
    }
    
    stats.total_bytes += bytes;
}

double Metrics::getBytesPerSecond(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = throughput_.find(name);
    
    if (it == throughput_.end() || it->second.total_bytes == 0) {
        return 0.0;
    }
    
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - it->second.start_time
    ).count();
    
    if (elapsed == 0) {
        return 0.0;
    }
    
    return static_cast<double>(it->second.total_bytes) / elapsed;
}

std::string Metrics::toString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::stringstream ss;
    
    ss << "=== Metrics ===\n";
    
    if (!counters_.empty()) {
        ss << "\nCounters:\n";
        for (const auto& [name, value] : counters_) {
            ss << "  " << name << ": " << value.load() << "\n";
        }
    }
    
    if (!gauges_.empty()) {
        ss << "\nGauges:\n";
        for (const auto& [name, value] : gauges_) {
            ss << "  " << name << ": " << value.load() << "\n";
        }
    }
    
    if (!latencies_.empty()) {
        ss << "\nLatencies:\n";
        for (const auto& [name, stats] : latencies_) {
            int64_t count = stats.count.load();
            if (count > 0) {
                double avg = static_cast<double>(stats.sum_us.load()) / count;
                ss << "  " << name << ":\n";
                ss << "    count: " << count << "\n";
                ss << "    avg: " << std::fixed << std::setprecision(2) << avg << " μs\n";
                ss << "    min: " << stats.min_us.load() << " μs\n";
                ss << "    max: " << stats.max_us.load() << " μs\n";
            }
        }
    }
    
    if (!throughput_.empty()) {
        ss << "\nThroughput:\n";
        for (const auto& [name, stats] : throughput_) {
            double bps = getBytesPerSecond(name);
            ss << "  " << name << ": " << std::fixed << std::setprecision(2) 
               << (bps / 1024.0 / 1024.0) << " MB/s\n";
        }
    }
    
    return ss.str();
}

void Metrics::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_.clear();
    gauges_.clear();
    latencies_.clear();
    throughput_.clear();
}

} // namespace dropboxlite
