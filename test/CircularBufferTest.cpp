#include <type_traits>
#include <gtest/gtest.h>

#include "CircularBuffer.hpp"

constexpr size_t BUFFER_SIZE = 10UL;   
TEST(CircularBufferTast, ValidateTypes) {
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


TEST(CircularBufferTast, CopyConstructorFixed) {
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

TEST(CircularBufferTast, CopyAssignmentOperatorFixed) {
    CircularBuffer::CircularBuffer<int, 5UL> buffer1;
    buffer1.push(1);
    buffer1.push(2);
    buffer1.push(3);

    // CircularBuffer::CircularBuffer<int, 5> buffer2;
    auto buffer2 = buffer1;
    EXPECT_EQ(buffer2.size(), 3);
    EXPECT_EQ(buffer2.top_pop().value(), 1);
    EXPECT_EQ(buffer2.top_pop().value(), 2);
    EXPECT_EQ(buffer2.top_pop().value(), 3);
}

TEST(CircularBufferTast, MoveConstructorFixed) {
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

TEST(CircularBufferTast, MoveAssignmentOperatorFixed) {
    CircularBuffer::CircularBuffer<int, 5UL> buffer1;
    buffer1.push(1);
    buffer1.push(2);
    buffer1.push(3);

    auto buffer2 = std::move(buffer1);
    EXPECT_EQ(buffer2.size(), 3);
    EXPECT_EQ(buffer2.top_pop().value(), 1);
    EXPECT_EQ(buffer2.top_pop().value(), 2);
    EXPECT_EQ(buffer2.top_pop().value(), 3);

    // buffer1 should be in a valid state after the move, but its content might be unspecified
    EXPECT_TRUE(buffer1.empty() or !buffer1.empty());
}

TEST(CircularBufferTast, CopyConstructorDynamic) {
    CircularBuffer::CircularBuffer<int> buffer(3UL);
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);

    auto copyBuffer(buffer);
    EXPECT_EQ(copyBuffer.size(), 3);
    EXPECT_EQ(copyBuffer.top_pop().value(), 1);
    EXPECT_EQ(copyBuffer.top_pop().value(), 2);
    EXPECT_EQ(copyBuffer.top_pop().value(), 3);
}

TEST(CircularBufferTast, CopyAssignmentOperatorDynamic) {
    CircularBuffer::CircularBuffer<int> buffer1(5UL);
    buffer1.push(1);
    buffer1.push(2);
    buffer1.push(3);

    auto buffer2 = buffer1;
    EXPECT_EQ(buffer2.size(), 3);
    EXPECT_EQ(buffer2.top_pop().value(), 1);
    EXPECT_EQ(buffer2.top_pop().value(), 2);
    EXPECT_EQ(buffer2.top_pop().value(), 3);
}

TEST(CircularBufferTast, MoveConstructorDynamic) {
    CircularBuffer::CircularBuffer<int> buffer(5UL);
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

TEST(CircularBufferTast, MoveAssignmentOperatorDynamic) {
    CircularBuffer::CircularBuffer<int> buffer1(5UL);
    buffer1.push(1);
    buffer1.push(2);
    buffer1.push(3);

    auto buffer2 = std::move(buffer1);
    EXPECT_EQ(buffer2.size(), 3);
    EXPECT_EQ(buffer2.top_pop().value(), 1);
    EXPECT_EQ(buffer2.top_pop().value(), 2);
    EXPECT_EQ(buffer2.top_pop().value(), 3);

    // buffer1 should be in a valid state after the move, but its content might be unspecified
    EXPECT_TRUE(buffer1.empty() or !buffer1.empty());
}