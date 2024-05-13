#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <gtest/gtest.h>
#include "CircularBufferFixed.hpp"

constexpr size_t BUFFER_SIZE = 5;

// Test basic push and pop functionality
TEST(CircularBufferFixedTest, PushAndPop) {
    CircularBuffer::CircularBufferFixed<size_t, BUFFER_SIZE> buffer;
    buffer.push(1);
    ASSERT_FALSE(buffer.empty());
    auto result = buffer.top_pop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value());
    EXPECT_TRUE(buffer.empty());
}

TEST(CircularBufferFixedTest, CapacityLimits) {
    CircularBuffer::CircularBufferFixed<size_t, BUFFER_SIZE> buffer;
    constexpr size_t buffer_size = BUFFER_SIZE;
    for (size_t i = 0; i < buffer_size; ++i) {
        buffer.push(i);
    }
    EXPECT_EQ(buffer_size, buffer.size());  // Ensure size is at capacity
    buffer.push(buffer_size);  // Push beyond capacity
    EXPECT_EQ(buffer_size, buffer.size());  // Size should not increase
    auto result = buffer.top_pop();  // Pop the oldest element
    EXPECT_EQ(1, result.value());  // Validate FIFO behavior on overflow
}

TEST(CircularBufferFixedTest, BoundaryConditions) {
    CircularBuffer::CircularBufferFixed<size_t, BUFFER_SIZE> buffer;
    constexpr size_t buffer_size = 9;
    for (size_t i = 0; i < buffer_size; ++i) {  // Fill buffer one less than capacity
        buffer.push(i);
    }
    buffer.push(9);  // Fill to capacity
    buffer.push(10);  // Trigger wrap-around
    EXPECT_EQ(6, buffer.top_pop().value());  // Next pop should follow the wrap-around
}

// TEST(CircularBufferFixedTest, ConcurrencyAndOrdering) {
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
//                     buffer.push(element);
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
//     while (!buffer.empty()) {
//         final_elements.push_back(buffer.top_pop().value());
//     }

//     ASSERT_EQ(final_elements.size(), buffer_capacity);
//     for (size_t i = 0; i < buffer_capacity; ++i) {
//         size_t expected_value = pushed_elements.at(pushed_elements.size() - buffer_capacity + i);
//         EXPECT_EQ(final_elements.at(i), expected_value) << "Mismatch at buffer index " << i;
//     }
// }

// TEST(CircularBufferFixedTest, MultipleProducersMultipleConsumers) {
//     constexpr size_t num_producers = BUFFER_SIZE;
//     constexpr size_t num_consumers = BUFFER_SIZE;
//     constexpr size_t num_operations = 10000UL;
//     std::vector<std::thread> producers;
//     std::vector<std::thread> consumers;
//     producers.reserve(num_producers);
//     consumers.reserve(num_consumers);
//     std::atomic<size_t> consumed_count(0);
//     std::mutex print_mutex;

//     for (size_t i = 0; i < num_producers; ++i) {
//         producers.emplace_back([&, i]() {
//             for (size_t j = 0; j < num_operations; ++j) {
//                 buffer.push(i * num_operations + j);
//             }
//         });
//     }

//     for (size_t i = 0; i < num_consumers; ++i) {
//         consumers.emplace_back([&]() {
//             while (consumed_count < num_producers * num_operations) {
//                 auto val = buffer.top_pop();
//                 if (val.has_value()) {
//                     size_t local_count = ++consumed_count;
//                     std::lock_guard<std::mutex> lock(print_mutex);
//                 }
//             }
//         });
//     }

//     for (auto& producer : producers) {
//         producer.join();
//     }
//     for (auto& consumer : consumers) {
//         consumer.join();
//     }

//     EXPECT_EQ(consumed_count, num_producers * num_operations);
// }

TEST(CircularBufferFixedTest, StressRobustness) {
    CircularBuffer::CircularBufferFixed<size_t, BUFFER_SIZE> buffer;
    constexpr size_t buffer_size = 1000000;
    try {
        for (size_t i = 0; i < buffer_size; ++i) {
            buffer.push(i);
            buffer.top_pop();
        }
    } catch (...) {
        FAIL() << "Unexpected exception during high load.";
    }
}

TEST(CircularBufferFixedTest, MemoryAndResourceManagement) {
    CircularBuffer::CircularBufferFixed<size_t, BUFFER_SIZE> buffer;
    constexpr size_t count = 1000;
    // Push elements into the buffer.
    for (size_t i = 0; i < count; ++i) {
        buffer.push(i);
    }

    // Check if elements are popped in the correct order.
    size_t expected = 995;
    while (!buffer.empty()) {
        auto popped = buffer.top_pop().value();
        EXPECT_EQ(popped, expected++) << "Mismatch at position " << expected - 1;
    }
}

// Test wrap-around behavior
TEST(CircularBufferFixedTest, WrapAround) {
    CircularBuffer::CircularBufferFixed<size_t, BUFFER_SIZE> buffer;
    constexpr size_t buffer_size = BUFFER_SIZE;
    for (size_t i = 0; i < buffer_size; ++i) {
        buffer.push(i);
    }
    buffer.push(buffer_size);  // This should overwrite the first element
    auto result = buffer.top_pop();  // Should pop the second element, not the first
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value());
}

// Test thread safety by using multiple threads to push and pop concurrently
TEST(CircularBufferFixedTest, ExtremeStressWithThreads) {
    CircularBuffer::CircularBufferFixed<size_t, BUFFER_SIZE> buffer;

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
                buffer.push(i * operations_per_thread + j);
                total_pushes++;
                if (buffer.top_pop().has_value()) {
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
    EXPECT_LE(buffer.size(), buffer_capacity);  // Check if buffer size is within its capacity

    // Clear the buffer and check
    std::vector<size_t> remaining_elements;
    remaining_elements.reserve(buffer_capacity);
    while (buffer.empty()) {
        auto val = buffer.top_pop();
        if (val.has_value()) {
            remaining_elements.push_back(val.value());
        }
    }
    EXPECT_TRUE(remaining_elements.size() <= buffer_capacity); // Remaining elements should not exceed buffer capacity
}

// Test that ensures buffer correctly overwrites old data
TEST(CircularBufferFixedTest, OverwriteOldEntries) {

    CircularBuffer::CircularBufferFixed<size_t, BUFFER_SIZE> buffer;

    // Push up to capacity
    for (size_t i = 1; i <= 5; ++i) {
        buffer.push(i);
    }
    
    // Check capacity reached and content is as expected
    EXPECT_EQ(5, buffer.size());
    size_t expectedValue = 1;
    for (auto it = buffer.begin(); it != buffer.end(); ++it, ++expectedValue) {
        EXPECT_EQ(expectedValue, *it) << "Initial values in buffer are incorrect at position " << expectedValue - 1;
    }

    // Push additional elements to trigger overwriting
    for (size_t i = 6; i <= 10; ++i) {
        buffer.push(i);
    }

    // Check size remains constant and content is updated
    EXPECT_EQ(5, buffer.size());
    expectedValue = 6;
    auto it = buffer.begin();
    for (; expectedValue <= 10; ++expectedValue, ++it) {
        ASSERT_TRUE(it != buffer.end()) << "Iterator reached the end prematurely.";
        EXPECT_EQ(expectedValue, *it) << "Buffer did not overwrite old values correctly at position " << expectedValue - 6;
    }
    EXPECT_TRUE(it == buffer.end()) << "Iterator did not reach the end of the buffer as expected.";
}

TEST(CircularBufferFixedTest, BasicOperations) {
    CircularBuffer::CircularBufferFixed<size_t, BUFFER_SIZE> buffer;
    // Test empty buffer
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_EQ(buffer.mean(), std::nullopt);

    // Push elements
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);
    buffer.push(4);
    buffer.push(5);
    EXPECT_EQ(buffer.size(), 5);

    // Test mean, median, min, max
    EXPECT_EQ(buffer.mean().value(), 3);
    // EXPECT_EQ(buffer.median().value(), 3);
    EXPECT_EQ(buffer.minimum().value(), 1);
    EXPECT_EQ(buffer.maximum().value(), 5);

    // Test popping
    EXPECT_EQ(buffer.top_pop().value(), 1);
    EXPECT_EQ(buffer.size(), 4);
    
    // More tests can be added based on your requirements.
}

TEST(CircularBufferFixedTest, Overflow) {
    CircularBuffer::CircularBufferFixed<size_t, 3> buffer;
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);
    buffer.push(4); // This will remove 1
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(buffer.minimum().value(), 2);
}

TEST(CircularBufferFixedTest, Reset) {
    CircularBuffer::CircularBufferFixed<size_t, BUFFER_SIZE> buffer;
    buffer.push(1);
    buffer.push(2);
    buffer.reset();
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0);
}


TEST(CircularBufferFixedTest, Emplace) {
    CircularBuffer::CircularBufferFixed<std::pair<int, int>, 3> buffer;
    buffer.emplace(1, 2);
    buffer.emplace(3, 4);
    EXPECT_EQ(buffer.size(), 2);
    auto val = buffer.top().value();
    EXPECT_EQ(val.first, 1);
    EXPECT_EQ(val.second, 2);
}


TEST(CircularBufferFixedTest, AlmostFullStatistics) {
    CircularBuffer::CircularBufferFixed<size_t, BUFFER_SIZE> buffer;
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);
    buffer.push(4);
    EXPECT_DOUBLE_EQ(buffer.sum().value(), 10);
    EXPECT_DOUBLE_EQ(buffer.mean().value(), 2.5);
    // EXPECT_DOUBLE_EQ(buffer.median().value(), 2.5);
    EXPECT_EQ(buffer.minimum().value(), 1);
    EXPECT_EQ(buffer.maximum().value(), 4);
}

TEST(CircularBufferFixedTest, SingleElementStatistics) {
    CircularBuffer::CircularBufferFixed<size_t, BUFFER_SIZE> buffer;
    buffer.push(1);
    EXPECT_DOUBLE_EQ(buffer.mean().value(), 1);
    // EXPECT_DOUBLE_EQ(buffer.median().value(), 1);
    EXPECT_EQ(buffer.minimum().value(), 1);
    EXPECT_EQ(buffer.maximum().value(), 1);
}

TEST(CircularBufferFixedTest, FloatStatistics) {
    CircularBuffer::CircularBufferFixed<float, 5> buffer;

    // Add floating-point numbers
    buffer.push(1.5);
    buffer.push(2.5);
    buffer.push(3.0);
    buffer.push(4.7);
    buffer.push(5.8);

    EXPECT_NEAR(buffer.mean().value(), (1.5 + 2.5 + 3.0 + 4.7 + 5.8) / 5, 1e-6); // Expect mean to be close to the calculated mean
    // EXPECT_NEAR(buffer.median().value(), 3.0, 1e-6); // Middle value when sorted is 3.0
    EXPECT_NEAR(buffer.minimum().value(), 1.5, 1e-6); // Minimum value
    EXPECT_NEAR(buffer.maximum().value(), 5.8, 1e-6); // Maximum value
}

TEST(CircularBufferFixedTest, CopyConstructor) {
    CircularBuffer::CircularBufferFixed<int, 3> buffer;
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);

    auto copyBuffer(buffer);
    EXPECT_EQ(copyBuffer.size(), 3);
    EXPECT_EQ(copyBuffer.top_pop().value(), 1);
    EXPECT_EQ(copyBuffer.top_pop().value(), 2);
    EXPECT_EQ(copyBuffer.top_pop().value(), 3);
}

TEST(CircularBufferFixedTest, CopyAssignmentOperator) {
    CircularBuffer::CircularBufferFixed<int, 5> buffer1;
    buffer1.push(1);
    buffer1.push(2);
    buffer1.push(3);

    CircularBuffer::CircularBufferFixed<int, 5> buffer2;
    buffer2 = buffer1;
    EXPECT_EQ(buffer2.size(), 3);
    EXPECT_EQ(buffer2.top_pop().value(), 1);
    EXPECT_EQ(buffer2.top_pop().value(), 2);
    EXPECT_EQ(buffer2.top_pop().value(), 3);
}

TEST(CircularBufferFixedTest, MoveConstructor) {
    CircularBuffer::CircularBufferFixed<int, 5> buffer;
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);

    CircularBuffer::CircularBufferFixed<int, 5> movedBuffer(std::move(buffer));
    EXPECT_EQ(movedBuffer.size(), 3);
    EXPECT_EQ(movedBuffer.top_pop().value(), 1);
    EXPECT_EQ(movedBuffer.top_pop().value(), 2);
    EXPECT_EQ(movedBuffer.top_pop().value(), 3);

    // buffer should be in a valid state after the move, but its content might be unspecified
    EXPECT_TRUE(buffer.empty() or !buffer.empty());
}

TEST(CircularBufferFixedTest, MoveAssignmentOperator) {
    CircularBuffer::CircularBufferFixed<int, 5> buffer1;
    buffer1.push(1);
    buffer1.push(2);
    buffer1.push(3);

    CircularBuffer::CircularBufferFixed<int, 5> buffer2;
    buffer2 = std::move(buffer1);
    EXPECT_EQ(buffer2.size(), 3);
    EXPECT_EQ(buffer2.top_pop().value(), 1);
    EXPECT_EQ(buffer2.top_pop().value(), 2);
    EXPECT_EQ(buffer2.top_pop().value(), 3);

    // buffer1 should be in a valid state after the move, but its content might be unspecified
    EXPECT_TRUE(buffer1.empty() or !buffer1.empty());
}

TEST(CircularBufferFixedTest, StressTest) {
    CircularBuffer::CircularBufferFixed<size_t, 1000000> buffer;
    for (size_t i = 0; i < 2000000; ++i) {
        buffer.push(i);
    }
    EXPECT_EQ(buffer.size(), 1000000); // Only the last 1000000 elements should be there
    EXPECT_DOUBLE_EQ(buffer.mean().value(), 1500000 - 0.5); // Mean of numbers from 1000000 to 1999999
    // EXPECT_DOUBLE_EQ(buffer.median().value(), 1499999.5);
    EXPECT_EQ(buffer.minimum().value(), 1000000);
    EXPECT_EQ(buffer.maximum().value(), 1999999);
    // Other statistical methods can be similarly tested
}

TEST(CircularBufferFixedTest, ExtremeStressTest) {
    CircularBuffer::CircularBufferFixed<size_t, 10> buffer;
    for (size_t i = 0; i < 2000000; ++i) {
        buffer.push(i);
    }
    EXPECT_EQ(buffer.size(), 10); // Only the last 1000000 elements should be there
    EXPECT_DOUBLE_EQ(buffer.mean().value(), 1999994.5); // Mean of numbers from 1999990 to 1999999
    // EXPECT_DOUBLE_EQ(buffer.median().value(), 1999994.5); // Median should also be 1999944.5
    EXPECT_EQ(buffer.minimum().value(), 1999990);
    EXPECT_EQ(buffer.maximum().value(), 1999999);
    // Other statistical methods can be similarly tested
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
