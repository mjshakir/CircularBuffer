#include <thread>
#include <vector>
#include <atomic>
#include <gtest/gtest.h>
#include "CircularBuffer.hpp"

template <typename T, size_t N>
class CircularBufferTest : public ::testing::Test {
protected:
    CircularBuffer<T, N> buffer;

    // Constructor that initializes the buffer based on whether it is dynamic or fixed.
    CircularBufferTest() {
        if constexpr (N == 0) {
            // Initialize dynamic buffer with an arbitrary size
            new (&buffer) CircularBuffer<T>(10);
        } else {
            // Initialize fixed buffer (requires no size specification)
            new (&buffer) CircularBuffer<T>();
        }
    }

    void SetUp() override {
        // Optional: reset buffer or configure before each test
    }

    void TearDown() override {
        // Optional: cleanup after tests
    }
};

// Define types to run tests with different configurations
using MyBufferTypes = ::testing::Types<
    CircularBuffer<int, 0>,  // Dynamic buffer with ints
    CircularBuffer<double, 10>  // Fixed buffer with doubles and size 10
>;
TYPED_TEST_SUITE(CircularBufferTest, MyBufferTypes);


// Test basic push and pop functionality
TYPED_TEST(CircularBufferTest, PushAndPop) {
    this->buffer.push(1);
    ASSERT_FALSE(this->buffer.empty());
    auto result = this->buffer.top_pop();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(1, result.value());
    EXPECT_TRUE(this->buffer.empty());
}

// Test reaching and exceeding capacity limits
TYPED_TEST(CircularBufferTest, CapacityLimits) {
    for (size_t i = 0; i < 10; ++i) {
        this->buffer.push(i);
    }
    EXPECT_EQ(10, this->buffer.size());  // Ensure size is correct
    this->buffer.push(10);  // Try to push beyond capacity, fixed behavior depends on N
    if constexpr (TypeParam::N == 0) {
        EXPECT_EQ(10, this->buffer.size());  // Dynamic should manage size
    } else {
        EXPECT_EQ(10, this->buffer.size());  // Fixed should not exceed size N
        auto result = this->buffer.top_pop();  // Pop one item
        EXPECT_EQ(1, result.value());  // Validate FIFO behavior after overflow
    }
}

// Test multithreading capability
TEST_F(CircularBufferTest<int, 10>, Multithreading) {
    std::vector<std::thread> producers;
    std::atomic<int> count(0);

    for (int i = 0; i < 5; ++i) {
        producers.emplace_back([&]() {
            for (int j = 0; j < 100; ++j) {
                this->buffer.push(j);
                auto val = this->buffer.top_pop();
                if (val.has_value()) {
                    count++;
                }
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    EXPECT_EQ(500, count);  // Ensure all push/pop operations were successful
}

// Test cleanup
TYPED_TEST(CircularBufferTest, Cleanup) {
    for (int i = 0; i < 10; ++i) {
        this->buffer.push(i);
    }
    this->buffer.clear();
    EXPECT_TRUE(this->buffer.empty());
}