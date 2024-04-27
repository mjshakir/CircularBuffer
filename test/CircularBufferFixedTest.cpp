#include <thread>
#include <vector>
#include <atomic>
#include <gtest/gtest.h>
#include "CircularBufferFixed.hpp"

// Test Fixture for CircularBufferFixed
class CircularBufferFixedSizeTest : public ::testing::Test {
protected:
    CircularBuffer::CircularBufferFixed<size_t, 5> buffer;  // Fixed size buffer
};

// Define types to run the same tests on different type specializations if needed
using MyBufferTypes = ::testing::Types<size_t, double>;
TYPED_TEST_SUITE(CircularBufferFixedTest, MyBufferTypes);

// Test basic push and pop functionality
TYPED_TEST(CircularBufferFixedTest, PushAndPop) {
    this->buffer.push(1);
    ASSERT_FALSE(this->buffer.empty());
    auto result = this->buffer.top_pop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value());
    EXPECT_TRUE(this->buffer.empty());
}

TYPED_TEST(CircularBufferFixedTest, CapacityLimits) {
    constexpr size_t buffer_size = 10;
    for (size_t i = 0; i < buffer_size; ++i) {
        this->buffer.push(i);
    }
    EXPECT_EQ(10, this->buffer.size());  // Ensure size is at capacity
    this->buffer.push(10);  // Push beyond capacity
    EXPECT_EQ(10, this->buffer.size());  // Size should not increase
    auto result = this->buffer.top_pop();  // Pop the oldest element
    EXPECT_EQ(1, result.value());  // Validate FIFO behavior on overflow
}

TYPED_TEST(CircularBufferFixedTest, BoundaryConditions) {
    constexpr size_t buffer_size = 9;
    for (size_t i = 0; i < buffer_size; ++i) {  // Fill buffer one less than capacity
        this->buffer.push(i);
    }
    this->buffer.push(9);  // Fill to capacity
    this->buffer.push(10);  // Trigger wrap-around
    EXPECT_EQ(0, this->buffer.top_pop().value());  // Next pop should follow the wrap-around
}


TEST_F(CircularBufferFixedSizeTest, ConcurrencyAndOrdering) {
    constexpr size_t num_threads = 10;
    constexpr size_t per_thread_count = 100;
    std::vector<std::thread> producers;
    producers.reserve(num_threads);
    std::atomic<bool> start{false};
    
    for (size_t i = 0; i < num_threads; ++i) {
        producers.emplace([&, i]() {
            while (!start) { std::this_thread::yield(); } // Wait for the start signal
            for (size_t j = 0; j < per_thread_count; ++j) {
                this->buffer.push(i * per_thread_count + j);
            }
        });
    }

    start = true; // Start all threads
    for (auto& th : producers) th.join();

    std::vector<int> results;
    while (!this->buffer.empty()) {
        results.push(this->buffer.top_pop().value());
    }

    EXPECT_EQ(num_threads * per_thread_count, results.size()); // Check all items are pushed and popped
    std::sort(results.begin(), results.end());
    for (int i = 0; i < num_threads * per_thread_count; ++i) {
        EXPECT_EQ(i, results[i]); // Check the integrity of pushed data
    }
}

TEST_F(CircularBufferFixedTest, StressRobustness) {
    constexpr size_t buffer_size = 1000000;
    try {
        for (size_t i = 0; i < buffer_size; ++i) {
            this->buffer.push(i);
            this->buffer.top_pop();
        }
    } catch (...) {
        FAIL() << "Unexpected exception during high load.";
    }
}

TYPED_TEST(CircularBufferFixedTest, MemoryAndResourceManagement) {
    for (TypeParam i = 0; i < 1000; ++i) {
        this->buffer.push(i); // TypeParam is the type used in the test instantiation
    }
    while (!this->buffer.empty()) {
        EXPECT_EQ(this->buffer.top_pop().value(), --i);
    }
}

// Test wrap-around behavior
TYPED_TEST(CircularBufferFixedTest, WrapAround) {
    constexpr size_t buffer_size = 10;
    for (size_t i = 0; i < buffer_size; ++i) {
        this->buffer.push(i);
    }
    this->buffer.push(buffer_size);  // This should overwrite the first element
    auto result = this->buffer.top_pop();  // Should pop the second element, not the first
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value());
}

// Test thread safety by using multiple threads to push and pop concurrently
TEST_F(CircularBufferFixedTest, ThreadSafety) {
    constexpr size_t num_threads = 5;
    constexpr size_t buffer_size = 100;
    constexpr size_t total_items = num_threads * buffer_size;
    std::vector<std::thread> producers, consumers;
    producers.reserve(num_threads);
    consumers.reserve(num_threads);
    std::atomic<size_t> total_produced{0}, total_consumed{0};

    // Start producers
    for (size_t i = 0; i < num_threads; ++i) {
        producers.emplace([&]() {
            for (size_t j = 0; j < buffer_size; ++j) {
                this->buffer.push(j);
                total_produced++;
            }
        });
    }

    // Start consumers
    for (size_t i = 0; i < num_threads; ++i) {
        consumers.emplace([&]() {
            while (total_consumed < total_items) {
                if (auto val = this->buffer.top_pop(); val.has_value()) {
                    total_consumed++;
                }
            }
        });
    }

    for (auto& th : producers) th.join();
    for (auto& th : consumers) th.join();

    EXPECT_EQ(total_items, total_produced);
    EXPECT_EQ(total_items, total_consumed);
}

// Stress test to ensure stability under high load
TEST_F(CircularBufferFixedTest, StressTest) {
    constexpr size_t buffer_size = 100000;
    for (int i = 0; i < buffer_size; ++i) {
        this->buffer.push(i);
        this->buffer.top_pop();
    }
}

// Extreme stress test with threads
TEST_F(CircularBufferFixedTest, ExtremeStressWithThreads) {
    constexpr size_t num_threads = 100;
    constexpr size_t buffer_size = 10000;

    std::vector<std::thread> workers;
    for (size_t i = 0; i < num_threads; ++i) {  // 100 threads for high concurrency
        workers.emplace([&]() {
            for (size_t j = 0; j < buffer_size; ++j) {  // Each thread does 10000 operations
                this->buffer.push(j);
                this->buffer.top_pop();
            }
        });
    }

    for (auto& th : workers) th.join();
}

// Test that ensures buffer correctly overwrites old data
TYPED_TEST(CircularBufferFixedTest, OverwriteOldEntries) {
    // Push up to capacity
    for (size_t i = 1; i <= 5; ++i) {
        this->buffer.push(i);
    }
    
    // Check capacity reached and content is as expected
    EXPECT_EQ(5, this->buffer.size());
    size_t expectedValue = 1;
    for (auto it = this->buffer.begin(); it != this->buffer.end(); ++it, ++expectedValue) {
        EXPECT_EQ(expectedValue, *it) << "Initial values in buffer are incorrect at position " << expectedValue - 1;
    }

    // Push additional elements to trigger overwriting
    for (size_t i = 6; i <= 10; ++i) {
        this->buffer.push(i);
    }

    // Check size remains constant and content is updated
    EXPECT_EQ(5, this->buffer.size());
    expectedValue = 6;
    auto it = this->buffer.begin();
    for (; expectedValue <= 10; ++expectedValue, ++it) {
        ASSERT_TRUE(it != this->buffer.end()) << "Iterator reached the end prematurely.";
        EXPECT_EQ(expectedValue, *it) << "Buffer did not overwrite old values correctly at position " << expectedValue - 6;
    }
    EXPECT_TRUE(it == this->buffer.end()) << "Iterator did not reach the end of the buffer as expected.";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
