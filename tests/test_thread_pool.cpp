#include "common/thread_pool.h"
#include <gtest/gtest.h>
#include <atomic>

using namespace dropboxlite;

TEST(ThreadPoolTest, BasicExecution) {
    ThreadPool pool(4);
    
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    
    for (int i = 0; i < 100; i++) {
        futures.push_back(pool.enqueue([&counter] {
            counter++;
        }));
    }
    
    for (auto& f : futures) {
        f.get();
    }
    
    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPoolTest, ReturnValues) {
    ThreadPool pool(2);
    
    auto future = pool.enqueue([] {
        return 42;
    });
    
    EXPECT_EQ(future.get(), 42);
}

TEST(ThreadPoolTest, Wait) {
    ThreadPool pool(4);
    
    std::atomic<int> counter{0};
    
    for (int i = 0; i < 10; i++) {
        pool.enqueue([&counter] {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            counter++;
        });
    }
    
    pool.wait();
    EXPECT_EQ(counter.load(), 10);
}
