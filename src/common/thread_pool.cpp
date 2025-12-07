#include "common/thread_pool.h"

namespace dropboxlite {

ThreadPool::ThreadPool(size_t num_threads) : stop_(false), active_tasks_(0) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    condition_.wait(lock, [this] {
                        return stop_ || !tasks_.empty();
                    });
                    
                    if (stop_ && tasks_.empty()) {
                        return;
                    }
                    
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                
                active_tasks_++;
                task();
                active_tasks_--;
                
                wait_condition_.notify_all();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    
    condition_.notify_all();
    
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

size_t ThreadPool::pending() const {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    return tasks_.size();
}

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    wait_condition_.wait(lock, [this] {
        return tasks_.empty() && active_tasks_ == 0;
    });
}

} // namespace dropboxlite
