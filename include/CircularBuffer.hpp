#pragma once

//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <iostream>
#include <type_traits>
//--------------------------------------------------------------
// User Defined Headers
//--------------------------------------------------------------
#include "CircularBufferFixed.hpp"
#include "CircularBufferDynamic.hpp"
//--------------------------------------------------------------
/**
 * @namespace CircularBuffer
 * @brief Namespace for the CircularBuffer implementation.
 */
namespace CircularBuffer {
    //--------------------------------------------------------------
    /**
     * @class CircularBuffer
     * @brief A circular buffer implementation that can be either fixed-size or dynamic-size.
     *
     * This class provides a circular buffer that can either be fixed-size (using CircularBufferFixed) or 
     * dynamic-size (using CircularBufferDynamic) depending on the template parameters. If the size N is set 
     * to 0, it will use a dynamic-size buffer. Otherwise, it will use a fixed-size buffer.
     *
     * @tparam T Type of elements stored in the buffer.
     * @tparam N Size of the buffer. If N is 0, a dynamic-size buffer is used.
     *
     * @example usage:
     * @code
     * #include <iostream>
     * #include "CircularBuffer.h"
     * 
     * int main() {
     *     // Fixed-size buffer
     *     CircularBuffer::CircularBuffer<int, 5> fixedBuffer;
     *     fixedBuffer.push(1);
     *     fixedBuffer.push(2);
     *     fixedBuffer.push(3);
     *     fixedBuffer.push(4);
     *     fixedBuffer.push(5);
     * 
     *     auto topFixed = fixedBuffer.top();
     *     if (topFixed) {
     *         std::cout << "Top element in fixed buffer: " << *topFixed << std::endl; // Output: Top element in fixed buffer: 1
     *     }
     * 
     *     // Dynamic-size buffer
     *     CircularBuffer::CircularBuffer<int> dynamicBuffer(3);
     *     dynamicBuffer.push(1);
     *     dynamicBuffer.push(2);
     *     dynamicBuffer.push(3);
     * 
     *     auto topDynamic = dynamicBuffer.top();
     *     if (topDynamic) {
     *         std::cout << "Top element in dynamic buffer: " << *topDynamic << std::endl; // Output: Top element in dynamic buffer: 1
     *     }
     * 
     *     return 0;
     * }
     * @endcode
     */
    template <typename T, size_t N = 0>
    class CircularBuffer : public std::conditional_t<N == 0, CircularBufferDynamic<T>, CircularBufferFixed<T, N>> {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            /**
             * @brief Deleted constructor for dynamic-size buffer.
             *
             * This constructor is deleted to prevent instantiation of a dynamic-size buffer without specifying the size.
             *
             * @tparam M Template parameter for buffer size, should be 0 for dynamic-size buffer.
             */
            template <size_t M = N, std::enable_if_t<M == 0, int> = 0>
            CircularBuffer(void)                                    = delete;
            //--------------------------
            /**
             * @brief Default constructor for fixed-size buffer.
             *
             * This constructor initializes a fixed-size circular buffer.
             *
             * @tparam M Template parameter for buffer size, should not be 0 for fixed-size buffer.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
             * buffer.push(1);
             * buffer.push(2);
             * @endcode
             */
            template <size_t M = N, std::enable_if_t<M != 0, int> = 0>
            CircularBuffer(void) : CircularBufferFixed<T, N>() {
                //--------------------------
            }// end CircularBuffer(void)
            //--------------------------
            /**
             * @brief Constructor for dynamic-size buffer.
             *
             * This constructor initializes a dynamic-size circular buffer with the specified size.
             *
             * @param size The maximum size of the buffer.
             * @tparam M Template parameter for buffer size, should be 0 for dynamic-size buffer.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int> buffer(3);
             * buffer.push(1);
             * buffer.push(2);
             * @endcode
             */
            template <size_t M = N, std::enable_if_t<M == 0, int> = 0>
            explicit CircularBuffer(const size_t& size) : CircularBufferDynamic<T>(size) {
                //--------------------------
            }// end explicit CircularBuffer(size_t size)
            //--------------------------
            /**
             * @brief Move constructor.
             *
             * This constructor initializes the buffer by moving the values from another CircularBuffer instance.
             *
             * @param other The CircularBuffer instance to move from.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer1;
             * buffer1.push(1);
             * 
             * CircularBuffer::CircularBuffer<int, 5> buffer2 = std::move(buffer1);
             * @endcode
             */
            CircularBuffer(CircularBuffer&& other)                  = default;
            //--------------------------
            /**
             * @brief Copy constructor.
             *
             * This constructor initializes the buffer with the values from another CircularBuffer instance.
             *
             * @param other The CircularBuffer instance to copy from.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer1;
             * buffer1.push(1);
             * 
             * CircularBuffer::CircularBuffer<int, 5> buffer2 = buffer1;
             * @endcode
             */
            CircularBuffer(const CircularBuffer& other)             = default;
            //--------------------------
            /**
             * @brief Move assignment operator.
             *
             * This operator moves the values from another CircularBuffer instance to this instance.
             *
             * @param other The CircularBuffer instance to move from.
             * @return A reference to this CircularBuffer instance.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer1;
             * buffer1.push(1);
             * 
             * CircularBuffer::CircularBuffer<int, 5> buffer2;
             * buffer2 = std::move(buffer1);
             * @endcode
             */
            CircularBuffer& operator=(CircularBuffer&& other)       = default;
            //--------------------------
            /**
             * @brief Copy assignment operator.
             *
             * This operator copies the values from another CircularBuffer instance to this instance.
             *
             * @param other The CircularBuffer instance to copy from.
             * @return A reference to this CircularBuffer instance.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer1;
             * buffer1.push(1);
             * 
             * CircularBuffer::CircularBuffer<int, 5> buffer2;
             * buffer2 = buffer1;
             * @endcode
             */
            CircularBuffer& operator=(const CircularBuffer& other)  = default;
            //--------------------------
            ~CircularBuffer(void)                                   = default;
            //--------------------------------------------------------------
    };// end class CircularBuffer
    //--------------------------------------------------------------
}// end namespace CircularBuffer
//--------------------------------------------------------------