#include <type_traits>
#include <gtest/gtest.h>

#include "CircularBuffer.hpp"

constexpr size_t BUFFER_SIZE = 10;   
TEST(CircularBufferTypeCheck, ValidateTypes) {
    // These checks are compile-time, this test will only run if they pass
    EXPECT_FALSE((std::is_same<CircularBuffer::CircularBuffer<int, BUFFER_SIZE>, CircularBuffer::CircularBufferFixed<int, BUFFER_SIZE>>::value));
    EXPECT_FALSE((std::is_same<CircularBuffer::CircularBuffer<int>, CircularBuffer::CircularBufferDynamic<int>>::value));

    // These checks are compile-time, this test will only run if they pass
    EXPECT_TRUE((std::is_base_of<CircularBuffer::CircularBufferFixed<int, BUFFER_SIZE>, CircularBuffer::CircularBuffer<int, BUFFER_SIZE>>::value));
    EXPECT_TRUE((std::is_base_of<CircularBuffer::CircularBufferDynamic<int>, CircularBuffer::CircularBuffer<int>>::value));

    // Optionally, you can force a runtime check to ensure the types are indeed what they are expected to be
    CircularBuffer::CircularBuffer<int, BUFFER_SIZE> fixedBuffer;
    CircularBuffer::CircularBuffer<int> dynamicBuffer(BUFFER_SIZE); // Make sure constructor parameters are correct

    
    EXPECT_NE(nullptr, (dynamic_cast<CircularBuffer::CircularBufferFixed<int, BUFFER_SIZE>*>(&fixedBuffer)));
    EXPECT_NE(nullptr, (dynamic_cast<CircularBuffer::CircularBufferDynamic<int>*>(&dynamicBuffer)));
}
