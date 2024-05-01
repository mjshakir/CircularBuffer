#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <gtest/gtest.h>
#include "CircularBufferFixed.hpp"

constexpr size_t BUFFER_SIZE = 5;
// Test Fixture for CircularBufferFixed
class CircularBufferFixedTest : public ::testing::Test {
protected:
    CircularBuffer::CircularBufferFixed<size_t, BUFFER_SIZE> buffer;  // Fixed size buffer

    void SetUp(void) override {
        buffer.reset(); // Clear buffer before each test
    }
};

// Test basic push and pop functionality
TEST_F(CircularBufferFixedTest, PushAndPop) {
    this->buffer.push(1);
    ASSERT_FALSE(this->buffer.empty());
    auto result = this->buffer.top_pop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value());
    EXPECT_TRUE(this->buffer.empty());
}

TEST_F(CircularBufferFixedTest, CapacityLimits) {
    constexpr size_t buffer_size = 5;
    for (size_t i = 0; i < buffer_size; ++i) {
        this->buffer.push(i);
    }
    EXPECT_EQ(buffer_size, this->buffer.size());  // Ensure size is at capacity
    this->buffer.push(buffer_size);  // Push beyond capacity
    EXPECT_EQ(buffer_size, this->buffer.size());  // Size should not increase
    auto result = this->buffer.top_pop();  // Pop the oldest element
    EXPECT_EQ(1, result.value());  // Validate FIFO behavior on overflow
}

TEST_F(CircularBufferFixedTest, BoundaryConditions) {
    constexpr size_t buffer_size = 9;
    for (size_t i = 0; i < buffer_size; ++i) {  // Fill buffer one less than capacity
        this->buffer.push(i);
    }
    this->buffer.push(9);  // Fill to capacity
    this->buffer.push(10);  // Trigger wrap-around
    EXPECT_EQ(6, this->buffer.top_pop().value());  // Next pop should follow the wrap-around
}

// TEST_F(CircularBufferFixedTest, ConcurrencyAndOrdering) {
//     constexpr size_t num_threads = 2;
//     constexpr size_t per_thread_count = 10;
//     constexpr size_t buffer_capacity = BUFFER_SIZE; // Assuming BUFFER_SIZE is 5
//     std::vector<std::thread> producers;
//     std::vector<size_t> pushed_elements(num_threads * per_thread_count);
//     producers.reserve(num_threads);
//     {
//         for (size_t i = 0; i < num_threads; ++i) {
//             producers.emplace_back([&, i]() {
//                 for (size_t j = 0; j < per_thread_count; ++j) {
//                     size_t element = i * per_thread_count + j;
//                     this->buffer.push(element);
//                     pushed_elements[i * per_thread_count + j] = element;
//                 }
//             });
//         }

//         for (auto& th : producers) {
//             th.join();
//         }
//     }
//     std::vector<size_t> final_elements;
//     final_elements.reserve(buffer_capacity);
//     while (!this->buffer.empty()) {
//         final_elements.push_back(this->buffer.top_pop().value());
//     }

//     ASSERT_EQ(final_elements.size(), buffer_capacity);
//     for (size_t i = 0; i < buffer_capacity; ++i) {
//         size_t expected_value = pushed_elements.at(pushed_elements.size() - buffer_capacity + i);
//         EXPECT_EQ(final_elements.at(i), expected_value) << "Mismatch at buffer index " << i;
//     }
// }

TEST_F(CircularBufferFixedTest, MultipleProducersMultipleConsumers) {
    constexpr size_t num_producers = BUFFER_SIZE;
    constexpr size_t num_consumers = BUFFER_SIZE;
    constexpr size_t num_operations = 10000UL;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    producers.reserve(num_producers);
    consumers.reserve(num_consumers);
    std::atomic<size_t> consumed_count(0);
    std::mutex print_mutex;

    for (size_t i = 0; i < num_producers; ++i) {
        producers.emplace_back([&, i]() {
            for (size_t j = 0; j < num_operations; ++j) {
                buffer.push(i * num_operations + j);
            }
        });
    }

    for (size_t i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            while (consumed_count < num_producers * num_operations) {
                auto val = buffer.top_pop();
                if (val.has_value()) {
                    size_t local_count = ++consumed_count;
                    std::lock_guard<std::mutex> lock(print_mutex);
                }
            }
        });
    }

    for (auto& producer : producers) {
        producer.join();
    }
    for (auto& consumer : consumers) {
        consumer.join();
    }

    EXPECT_EQ(consumed_count, num_producers * num_operations);
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

TEST_F(CircularBufferFixedTest, MemoryAndResourceManagement) {
    constexpr size_t count = 1000;
    // Push elements into the buffer.
    for (size_t i = 0; i < count; ++i) {
        this->buffer.push(i);
    }

    // Check if elements are popped in the correct order.
    size_t expected = 995;
    while (!this->buffer.empty()) {
        auto popped = this->buffer.top_pop().value();
        EXPECT_EQ(popped, expected++) << "Mismatch at position " << expected - 1;
    }
}

// Test wrap-around behavior
TEST_F(CircularBufferFixedTest, WrapAround) {
    constexpr size_t buffer_size = BUFFER_SIZE;
    for (size_t i = 0; i < buffer_size; ++i) {
        this->buffer.push(i);
    }
    this->buffer.push(buffer_size);  // This should overwrite the first element
    auto result = this->buffer.top_pop();  // Should pop the second element, not the first
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value());
}

// Test thread safety by using multiple threads to push and pop concurrently
TEST_F(CircularBufferFixedTest, ExtremeStressWithThreads) {
    constexpr size_t num_threads = 100;
    constexpr size_t operations_per_thread = 10000;  // Each thread does 5000 pushes and 5000 pops
    constexpr size_t buffer_capacity = BUFFER_SIZE;  // Fixed buffer size

    std::vector<std::thread> workers;
    workers.reserve(num_threads);
    std::atomic<size_t> total_pops{0};
    std::atomic<size_t> total_pushes{0};

    // Launch threads to simultaneously push to and pop from the buffer
    for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back([&, i]() {
            for (size_t j = 0; j < operations_per_thread / 2; ++j) {
                // Push and immediately try to pop
                this->buffer.push(i * operations_per_thread + j);
                total_pushes++;
                if (this->buffer.top_pop().has_value()) {
                    total_pops++;
                }
            }
        });
    }

    for (auto& th : workers) {
        th.join();
    }

    // After all operations, check pops and pushes
    EXPECT_EQ(total_pushes.load(), num_threads * (operations_per_thread / 2));
    EXPECT_LE(total_pops.load(), total_pushes.load()); // Pops should be less or equal to pushes

    // The buffer should either be full or have fewer items depending on the last operation's timing
    EXPECT_LE(this->buffer.size(), buffer_capacity);  // Check if buffer size is within its capacity

    // Clear the buffer and check
    std::vector<size_t> remaining_elements;
    remaining_elements.reserve(buffer_capacity);
    while (!this->buffer.empty()) {
        auto val = this->buffer.top_pop();
        if (val.has_value()) {
            remaining_elements.push_back(val.value());
        }
    }
    EXPECT_TRUE(remaining_elements.size() <= buffer_capacity); // Remaining elements should not exceed buffer capacity
}

// Test that ensures buffer correctly overwrites old data
TEST_F(CircularBufferFixedTest, OverwriteOldEntries) {
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
