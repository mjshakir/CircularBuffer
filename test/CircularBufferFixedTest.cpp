//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <condition_variable>
//--------------------------------------------------------------
// GTest library
//--------------------------------------------------------------
#include <gtest/gtest.h>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "CircularBuffer.hpp"

constexpr size_t BUFFER_SIZE = 5UL;

// Test basic push and pop functionality
TEST(CircularBufferFixedTest, PushAndPop) {
    CircularBuffer::CircularBuffer<size_t, BUFFER_SIZE> buffer;
    buffer.push(1);
    ASSERT_FALSE(buffer.empty());
    auto result = buffer.top_pop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value());
    EXPECT_TRUE(buffer.empty());
}

TEST(CircularBufferFixedTest, CapacityLimits) {
    CircularBuffer::CircularBuffer<size_t, BUFFER_SIZE> buffer;
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
    CircularBuffer::CircularBuffer<size_t, BUFFER_SIZE> buffer;
    constexpr size_t buffer_size = 9;
    for (size_t i = 0; i < buffer_size; ++i) {  // Fill buffer one less than capacity
        buffer.push(i);
    }
    buffer.push(9);  // Fill to capacity
    buffer.push(10);  // Trigger wrap-around
    EXPECT_EQ(6, buffer.top_pop().value());  // Next pop should follow the wrap-around
}


TEST(CircularBufferFixedTest, StressRobustness) {
    CircularBuffer::CircularBuffer<size_t, BUFFER_SIZE> buffer;
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
    CircularBuffer::CircularBuffer<size_t, BUFFER_SIZE> buffer;
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
    CircularBuffer::CircularBuffer<size_t, BUFFER_SIZE> buffer;
    constexpr size_t buffer_size = BUFFER_SIZE;
    for (size_t i = 0; i < buffer_size; ++i) {
        buffer.push(i);
    }
    buffer.push(buffer_size);  // This should overwrite the first element
    auto result = buffer.top_pop();  // Should pop the second element, not the first
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value());
}

// Test that ensures buffer correctly overwrites old data
TEST(CircularBufferFixedTest, OverwriteOldEntries) {

    CircularBuffer::CircularBuffer<size_t, BUFFER_SIZE> buffer;

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
    CircularBuffer::CircularBuffer<size_t, BUFFER_SIZE> buffer;
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
    EXPECT_EQ(buffer.sum().value(), 15);
    EXPECT_EQ(buffer.mean().value(), 3);
    EXPECT_NEAR(buffer.variance().value(), 2.5, 1e-6); 
    EXPECT_NEAR(buffer.standard_deviation().value(), std::sqrt(2.5), 1e-6);
    EXPECT_EQ(buffer.median().value(), 3);
    EXPECT_EQ(buffer.minimum().value(), 1);
    EXPECT_EQ(buffer.maximum().value(), 5);

    // Test popping
    EXPECT_EQ(buffer.top_pop().value(), 1);
    EXPECT_EQ(buffer.size(), 4);
    
    // More tests can be added based on your requirements.
}

TEST(CircularBufferFixedTest, Overflow) {
    CircularBuffer::CircularBuffer<size_t, 3UL> buffer;
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);
    buffer.push(4); // This will remove 1
    
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_EQ(buffer.sum().value(), 9);
    EXPECT_EQ(buffer.mean().value(), 3);
    EXPECT_NEAR(buffer.variance().value(), 1, 1e-6);
    EXPECT_NEAR(buffer.standard_deviation().value(), 1, 1e-6);
    EXPECT_EQ(buffer.minimum().value(), 2);
    EXPECT_EQ(buffer.maximum().value(), 4);
}

TEST(CircularBufferFixedTest, Reset) {
    CircularBuffer::CircularBuffer<size_t, BUFFER_SIZE> buffer;
    buffer.push(1);
    buffer.push(2);
    buffer.reset();
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0);
}


TEST(CircularBufferFixedTest, Emplace) {
    CircularBuffer::CircularBuffer<std::pair<int, int>, 3UL> buffer;
    buffer.emplace(1, 2);
    buffer.emplace(3, 4);
    EXPECT_EQ(buffer.size(), 2);
    auto val = buffer.top().value();
    EXPECT_EQ(val.first, 1);
    EXPECT_EQ(val.second, 2);
    auto last = buffer.last().value();
    EXPECT_EQ(last.first, 3);
    EXPECT_EQ(last.second, 4);
}


TEST(CircularBufferFixedTest, AlmostFullStatistics) {
    CircularBuffer::CircularBuffer<size_t, BUFFER_SIZE> buffer;
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);
    buffer.push(4);
    EXPECT_DOUBLE_EQ(buffer.sum().value(), 10);
    EXPECT_DOUBLE_EQ(buffer.mean().value(), 2.5);
    EXPECT_NEAR(buffer.variance().value(), 1.6666666666666667, 1e-6);
    EXPECT_NEAR(buffer.standard_deviation().value(), 1.2909944487358056, 1e-6);
    EXPECT_DOUBLE_EQ(buffer.median().value(), 2.5);
    EXPECT_EQ(buffer.minimum().value(), 1);
    EXPECT_EQ(buffer.maximum().value(), 4);

}

TEST(CircularBufferFixedTest, SingleElementStatistics) {
    CircularBuffer::CircularBuffer<size_t, BUFFER_SIZE> buffer;
    buffer.push(1);
    EXPECT_DOUBLE_EQ(buffer.sum().value(), 1);
    EXPECT_DOUBLE_EQ(buffer.mean().value(), 1);
    EXPECT_DOUBLE_EQ(buffer.median().value(), 1);
    EXPECT_EQ(buffer.minimum().value(), 1);
    EXPECT_EQ(buffer.maximum().value(), 1);
}

TEST(CircularBufferFixedTest, FloatStatistics) {
    CircularBuffer::CircularBuffer<float, BUFFER_SIZE> buffer;

    // Add floating-point numbers
    buffer.push(1.5);
    buffer.push(2.5);
    buffer.push(3.0);
    buffer.push(4.7);
    buffer.push(5.8);

    constexpr double expected_mean = (1.5 + 2.5 + 3.0 + 4.7 + 5.8) / 5;

    EXPECT_NEAR(buffer.sum().value(), 1.5 + 2.5 + 3.0 + 4.7 + 5.8, 1e-6); // Expect sum to be close to the calculated sum
    EXPECT_NEAR(buffer.mean().value(), expected_mean, 1e-6); // Expect mean to be close to the calculated mean
    // Calculate the expected variance and standard deviation with Bessel's correction
    const double expected_variance = ((std::pow(1.5 - expected_mean, 2) + std::pow(2.5 - expected_mean, 2) +
                                 std::pow(3.0 - expected_mean, 2) + std::pow(4.7 - expected_mean, 2) + 
                                 std::pow(5.8 - expected_mean, 2)) / 4); // Using N-1
    const double expected_std_dev = std::sqrt(expected_variance);

    EXPECT_NEAR(buffer.variance().value(), expected_variance, 1e-5);
    EXPECT_NEAR(buffer.standard_deviation().value(), expected_std_dev, 1e-6);
    EXPECT_NEAR(buffer.median().value(), 3.0, 1e-6); // Middle value when sorted is 3.0
    EXPECT_NEAR(buffer.minimum().value(), 1.5, 1e-6); // Minimum value
    EXPECT_NEAR(buffer.maximum().value(), 5.8, 1e-6); // Maximum value
}

TEST(CircularBufferFixedTest, CopyConstructor) {
    CircularBuffer::CircularBuffer<int, 3UL> buffer;
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
    CircularBuffer::CircularBuffer<int, 5UL> buffer1;
    buffer1.push(1);
    buffer1.push(2);
    buffer1.push(3);

    // CircularBuffer::CircularBufferFixed<int, 5> buffer2;
    auto buffer2 = buffer1;
    EXPECT_EQ(buffer2.size(), 3);
    EXPECT_EQ(buffer2.top_pop().value(), 1);
    EXPECT_EQ(buffer2.top_pop().value(), 2);
    EXPECT_EQ(buffer2.top_pop().value(), 3);
}

TEST(CircularBufferFixedTest, MoveConstructor) {
    CircularBuffer::CircularBuffer<int, 5UL> buffer;
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);

    auto movedBuffer(std::move(buffer));
    EXPECT_EQ(movedBuffer.size(), 3);
    EXPECT_EQ(movedBuffer.top_pop().value(), 1);
    EXPECT_EQ(movedBuffer.top_pop().value(), 2);
    EXPECT_EQ(movedBuffer.top_pop().value(), 3);

    // buffer should be in a valid state after the move, but its content might be unspecified
    EXPECT_TRUE(buffer.empty() or !buffer.empty());
}

TEST(CircularBufferFixedTest, MoveAssignmentOperator) {
    CircularBuffer::CircularBuffer<int, 5UL> buffer1;
    buffer1.push(1);
    buffer1.push(2);
    buffer1.push(3);

    // CircularBuffer::CircularBufferFixed<int, 5> buffer2;
    auto buffer2 = std::move(buffer1);
    EXPECT_EQ(buffer2.size(), 3);
    EXPECT_EQ(buffer2.top_pop().value(), 1);
    EXPECT_EQ(buffer2.top_pop().value(), 2);
    EXPECT_EQ(buffer2.top_pop().value(), 3);

    // buffer1 should be in a valid state after the move, but its content might be unspecified
    EXPECT_TRUE(buffer1.empty() or !buffer1.empty());
}

TEST(CircularBufferFixedTest, ExtremeStressTest) {
    CircularBuffer::CircularBuffer<size_t, 10UL> buffer;
    for (size_t i = 0; i < 2000000; ++i) {
        buffer.push(i);
    }
    EXPECT_EQ(buffer.size(), 10); // Only the last 1000000 elements should be there
    EXPECT_EQ(buffer.sum().value(), 19999945); // Sum of numbers from 1999990 to 1999999
    EXPECT_DOUBLE_EQ(buffer.mean().value(), 1999994.5); // Mean of numbers from 1999990 to 1999999
    // Calculate the expected variance and standard deviation with Bessel's correction
    constexpr double expected_mean = 1999994.5;
    const double expected_variance =    ((std::pow(1999990 - expected_mean, 2) + std::pow(1999991 - expected_mean, 2) +
                                        std::pow(1999992 - expected_mean, 2) + std::pow(1999993 - expected_mean, 2) +
                                        std::pow(1999994 - expected_mean, 2) + std::pow(1999995 - expected_mean, 2) +
                                        std::pow(1999996 - expected_mean, 2) + std::pow(1999997 - expected_mean, 2) +
                                        std::pow(1999998 - expected_mean, 2) + std::pow(1999999 - expected_mean, 2)) / 9); // Using N-1
    const double expected_std_dev = std::sqrt(expected_variance);

    EXPECT_NEAR(buffer.variance().value(), expected_variance, 1e-6);
    EXPECT_NEAR(buffer.standard_deviation().value(), expected_std_dev, 1e-6);
    EXPECT_DOUBLE_EQ(buffer.median().value(), 1999994.5); // Median should also be 1999944.5
    EXPECT_EQ(buffer.minimum().value(), 1999990);
    EXPECT_EQ(buffer.maximum().value(), 1999999);
    // Other statistical methods can be similarly tested
}

TEST(CircularBufferFixedTest, ExtremeStressTestDouble) {
    CircularBuffer::CircularBuffer<double, 10UL> buffer;
    for (size_t i = 0; i < 2000000; ++i) {
        buffer.push(static_cast<double>(i) + 0.5);
    }

    constexpr double expected_mean = 1999995.0;
    EXPECT_EQ(buffer.size(), 10UL); // Only the last 10 elements should be there
    EXPECT_DOUBLE_EQ(buffer.sum().value(), 19999950.0); // Sum of numbers from 1999990.5 to 1999999.5
    EXPECT_DOUBLE_EQ(buffer.mean().value(), expected_mean); // Mean of numbers from 1999990.5 to 1999999.5
    // Calculate the expected variance and standard deviation with Bessel's correction
    const double expected_variance = ((std::pow(1999990.5 - expected_mean, 2) + std::pow(1999991.5 - expected_mean, 2) +
                                 std::pow(1999992.5 - expected_mean, 2) + std::pow(1999993.5 - expected_mean, 2) +
                                 std::pow(1999994.5 - expected_mean, 2) + std::pow(1999995.5 - expected_mean, 2) +
                                 std::pow(1999996.5 - expected_mean, 2) + std::pow(1999997.5 - expected_mean, 2) +
                                 std::pow(1999998.5 - expected_mean, 2) + std::pow(1999999.5 - expected_mean, 2)) / 9); // Using N-1
    const double expected_std_dev = std::sqrt(expected_variance);

    EXPECT_NEAR(buffer.variance().value(), expected_variance, 1e-6);
    EXPECT_NEAR(buffer.standard_deviation().value(), expected_std_dev, 1e-6);
    EXPECT_DOUBLE_EQ(buffer.median().value(), 1999995.0); // Median should also be 1999995.0
    EXPECT_DOUBLE_EQ(buffer.minimum().value(), 1999990.5);
    EXPECT_DOUBLE_EQ(buffer.maximum().value(), 1999999.5);
    // Other statistical methods can be similarly tested
}

TEST(CircularBufferFixedTest, StressTest) {
#ifdef _WIN32
    constexpr size_t buffer_size = 50000UL;
    CircularBuffer::CircularBuffer<size_t, buffer_size> buffer;
    for (size_t i = 0; i < 2000000UL; ++i) {
        buffer.push(i);
    }
#else
    constexpr size_t buffer_size = 500000UL;
    CircularBuffer::CircularBuffer<size_t, buffer_size> buffer;
    for (size_t i = 0; i < 2000000UL; ++i) {
        buffer.push(i);
    }
#endif
    EXPECT_EQ(buffer.size(), buffer_size); // Only the last 1000000 elements should be there

    constexpr size_t end_value      = 1999999UL;
    constexpr size_t start_value    = (end_value - buffer_size + 1UL);
    constexpr size_t num_elements   = end_value - start_value + 1UL;

    constexpr size_t expected_sum   = (num_elements * (start_value + end_value)) / 2UL; // Sum of numbers from 1000000 to 1999999
    constexpr double expected_mean  = (start_value + end_value) / 2.; // Mean of numbers from 1000000 to 1999999

    EXPECT_EQ(buffer.sum().value(), expected_sum);
    EXPECT_DOUBLE_EQ(buffer.mean().value(), expected_mean);

    // Calculate the expected variance and standard deviation with Bessel's correction
    double variance_sum = 0.0;
    for (size_t i = start_value; i <= end_value; ++i) {
        variance_sum += std::pow(static_cast<double>(i) - expected_mean, 2);
    }
    const double expected_variance = variance_sum / (num_elements - 1);
    const double expected_std_dev = std::sqrt(expected_variance);

    EXPECT_NEAR(buffer.variance().value(), expected_variance, 1e1);
    EXPECT_NEAR(buffer.standard_deviation().value(), expected_std_dev, 1e-3);
    EXPECT_DOUBLE_EQ(buffer.median().value(), expected_mean);
    EXPECT_EQ(buffer.minimum().value(), start_value);
    EXPECT_EQ(buffer.maximum().value(), end_value);
    // Other statistical methods can be similarly tested
}

template <typename T>
void fill_buffer(T& buffer, const size_t& start, const size_t& end) {
    for (size_t i = start; i < end; ++i) {
        buffer.push(i);
    }
}

// Test helper function to empty the buffer
template <typename T>
void empty_buffer(T& buffer, const size_t& count) {
    for (size_t i = 0UL; i < count; ++i) {
        buffer.pop();
    }
}

TEST(CircularBufferFixedTest, SingleProducerSingleConsumer) {
    CircularBuffer::CircularBuffer<size_t, 100UL> buffer;
    std::atomic<bool> done(false);

    std::thread producer([&buffer, &done]() {
        fill_buffer(buffer, 0, 1000UL);
        done.store(true);
    });

    std::thread consumer([&buffer, &done]() {
        while (!done.load() or !buffer.empty()) {
            buffer.pop();
        }
    });

    producer.join();
    consumer.join();

    EXPECT_TRUE(buffer.empty());
}

// Multiple Producers, 1 Consumer
TEST(CircularBufferFixedTest, MultipleProducersSingleConsumer) {
    CircularBuffer::CircularBuffer<size_t, 100UL> buffer;
    std::atomic<size_t> produced_count(0);
    constexpr size_t items_to_produce = 1000UL;
    std::condition_variable cv;
    std::mutex mtx;
    bool ready = false;

    auto producer = [&](size_t start) {
        for (size_t i = start; i < start + items_to_produce / 2; ++i) {
            buffer.push(i);
            ++produced_count;
        }
        {
            std::lock_guard<std::mutex> lock(mtx);
            ready = true;
        }
        cv.notify_all();
    };

    std::thread producer1(producer, 0);
    std::thread producer2(producer, items_to_produce / 2);

    std::thread consumer([&]() {
        size_t consumed_count = 0;
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() { return ready; });

        while ((consumed_count < items_to_produce) and !buffer.empty()) {
            if (buffer.pop()) {
                ++consumed_count;
            }
        }
    });

    producer1.join();
    producer2.join();
    consumer.join();

    EXPECT_TRUE(buffer.empty());
}

// 1 Producer, Multiple Consumers
TEST(CircularBufferFixedTest, SingleProducerMultipleConsumers) {
    CircularBuffer::CircularBuffer<size_t, 100UL> buffer;
    constexpr size_t items_to_produce = 1000UL;
    std::atomic<size_t> consumed_count(0);
    std::atomic<size_t> produced_count(0);
    std::condition_variable cv;
    std::mutex mtx;
    bool ready = false;

    std::thread producer([&]() {
        for (size_t i = 0; i < items_to_produce; ++i) {
            buffer.push(i);
            ++produced_count;
        }
        {
            std::lock_guard<std::mutex> lock(mtx);
            ready = true;
        }
        cv.notify_all();
    });

    auto consumer = [&]() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() { return ready; });

        while ((consumed_count < items_to_produce) and !buffer.empty()) {
            if (buffer.pop()) {
                ++consumed_count;
            }
        }
    };

    std::thread consumer1(consumer);
    std::thread consumer2(consumer);

    producer.join();
    consumer1.join();
    consumer2.join();

    EXPECT_TRUE(buffer.empty());
}

// Multiple Producers, Multiple Consumers
TEST(CircularBufferFixedTest, MultipleProducersMultipleConsumers) {
    CircularBuffer::CircularBuffer<size_t, 100UL> buffer;
    constexpr size_t items_to_produce = 1000UL;
    std::atomic<size_t> produced_count(0UL);
    std::atomic<size_t> consumed_count(0UL);
    std::condition_variable cv;
    std::mutex mtx;
    bool ready = false;

    auto producer = [&](size_t start) {
        for (size_t i = start; i < start + items_to_produce / 2; ++i) {
            buffer.push(i);
            ++produced_count;
        }
        {
            std::lock_guard<std::mutex> lock(mtx);
            ready = true;
        }
        cv.notify_all();
    };

    auto consumer = [&]() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() { return ready; });

        while ((consumed_count < items_to_produce) and !buffer.empty()) {
            if (produced_count.load() > consumed_count.load() && buffer.pop()) {
                ++consumed_count;
            }
        }
    };

    std::thread producer1(producer, 0);
    std::thread producer2(producer, items_to_produce / 2);

    std::thread consumer1(consumer);
    std::thread consumer2(consumer);

    producer1.join();
    producer2.join();
    consumer1.join();
    consumer2.join();

    EXPECT_TRUE(buffer.empty());
}

// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
