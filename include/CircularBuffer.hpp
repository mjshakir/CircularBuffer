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
namespace CircularBuffer {
    //--------------------------------------------------------------
    template <typename T, size_t N = 0>
    class CircularBuffer : public std::conditional_t<N == 0, CircularBufferDynamic<T>, CircularBufferFixed<T, N>> {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            template <size_t M = N, std::enable_if_t<M == 0, int> = 0>
            CircularBuffer(void) = delete;
            //--------------------------
            template <size_t M = N, std::enable_if_t<M != 0, int> = 0>
            CircularBuffer(void) : CircularBufferFixed<T, N>>() {
                //--------------------------
            }// end CircularBuffer(void)
            //--------------------------
            template <size_t M = N, std::enable_if_t<M == 0, int> = 0>
            explicit CircularBuffer(const size_t& size) : CircularBufferDynamic<T>(size) {
                //--------------------------
            }// end explicit CircularBuffer(size_t size)
            //--------------------------
            ~CircularBuffer(void) = default;
            //--------------------------------------------------------------
    };// end class CircularBuffer
    //--------------------------------------------------------------
}// end namespace CircularBuffer
//--------------------------------------------------------------