#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <iostream>
#include <optional>
#include <array>
#include <atomic>
#include <span>
//--------------------------------------------------------------
// User Defined Headers
//--------------------------------------------------------------
#include "CircularBufferBased.hpp"
//--------------------------------------------------------------
namespace CircularBuffer {
    //--------------------------------------------------------------
    template <typename T, size_t N>
    class CircularBufferFixed : public CircularBufferBased<T> {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            CircularBufferFixed(void) : m_head(0), m_tail(0), m_count(0) {
                //--------------------------
            }// end CircularBufferFixed(void)
            //--------------------------
            ~CircularBufferFixed(void) = default;
            //--------------------------
            typename std::array<T, N>::iterator begin(void) {
                return m_buffer.begin();
            }//end typename std::array<T, N>::iterator begin(void)
            //--------------------------
            typename std::array<T, N>::iterator end(void) {
                return m_buffer.end();
            }// end typename std::array<T, N>::iterator
            //--------------------------
            typename std::array<T, N>::const_iterator begin(void) const {
                return m_buffer.begin();
            }// end typename std::array<T, N>::const_iterator begin(void) const
            //--------------------------
            typename std::array<T, N>::const_iterator end(void) const {
                return m_buffer.end();
            }//end typename std::array<T, N>::const_iterator end(void) const
            //--------------------------
            typename std::array<T, N>::const_iterator cbegin(void) const {
                return m_buffer.cbegin();
            }//end typename std::array<T, N>::const_iterator cbegin(void) const
            //--------------------------
            typename std::array<T, N>::const_iterator cend(void) const {
                return m_buffer.cend();
            }//end typename std::array<T, N>::const_iterator cend(void) const
            //--------------------------
            typename std::array<T, N>::reverse_iterator rbegin(void) {
                return m_buffer.rbegin();
            }//end typename std::array<T, N>::reverse_iterator rbegin(void)
            //--------------------------
            typename std::array<T, N>::reverse_iterator rend(void) {
                return m_buffer.rend();
            }//end typename std::array<T, N>::reverse_iterator rend(void)
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            void push_back(const T& item) override {
                //--------------------------
                size_t current_tail = m_tail.load(std::memory_order_relaxed);
                size_t next_tail = increment(current_tail);
                //--------------------------
                if (m_count.load(std::memory_order_acquire) == N) {  // Queue is full
                    //--------------------------
                    pop_front();  // Remove the oldest item to make space
                    //--------------------------
                }//end if (size.load(std::memory_order_acquire) == N)
                //--------------------------
                m_buffer[current_tail] = item;
                //--------------------------
                m_tail.store(next_tail, std::memory_order_release);
                m_count.fetch_add(1, std::memory_order_release);
                //--------------------------
            }//end bool push_back(const T& item)
            //--------------------------
            void push_back(T&& item) override {
                //--------------------------
                size_t current_tail = m_tail.load(std::memory_order_relaxed);
                size_t next_tail = increment(current_tail);
                //--------------------------
                if (m_count.load(std::memory_order_acquire) == N) {  // Queue is full
                    //--------------------------
                    pop_front();  // Remove the oldest item to make space
                    //--------------------------
                }//end if (size.load(std::memory_order_acquire) == N)
                //--------------------------
                m_buffer[current_tail] = std::move(item);
                //--------------------------
                m_tail.store(next_tail, std::memory_order_release);
                m_count.fetch_add(1, std::memory_order_release);
                //--------------------------
            }//end bool push_back(const T& item)
            //--------------------------
            template <typename... Args>
            void emplace_back(Args&&... args) override {
                //--------------------------
                do {
                    size_t current_tail = m_tail.load(std::memory_order_relaxed);
                    //--------------------------
                    size_t next_tail = increment(current_tail);
                    //--------------------------
                    if (m_count.load(std::memory_order_acquire) == N) {  // Buffer is full
                        pop_front();  // Attempt to clear space
                    }// end if (size.load(std::memory_order_acquire) == N)
                    //--------------------------
                } while (!m_tail.compare_exchange_weak(current_tail, next_tail, std::memory_order_release, std::memory_order_relaxed));
                //--------------------------
                new (&m_buffer[current_tail]) T(std::forward<Args>(args)...);  // Construct in-place
                //--------------------------
                m_count.fetch_add(1, std::memory_order_release);
                //--------------------------
            }// end void emplace_back(Args&&... args)
            //--------------------------
            std::optional<T> get_top(void) override {
                //--------------------------
                std::atomic_thread_fence(std::memory_order_acquire);  // Ensure visibility of writes
                //--------------------------
                size_t current_head = m_head.load(std::memory_order_relaxed), current_tail = m_tail.load(std::memory_order_relaxed);
                //--------------------------
                if (current_head == current_tail or is_empty()) {
                    return std::nullopt;  // Buffer is empty
                }//end if (current_head == current_tail)
                //--------------------------
                return m_buffer.at(current_head);  // Return the element at head
                //--------------------------
            }//end std::optional<T> get_top(void)
            //--------------------------
            std::optional<T> get_top_pop(void) override {
                //--------------------------
                size_t current_head = m_head.load(std::memory_order_relaxed), current_tail = m_tail.load(std::memory_order_relaxed);
                //--------------------------
                if (current_head == current_tail or is_empty()) {
                    return std::nullopt;  // Buffer is empty
                }// end if (current_head == current_tail)
                //--------------------------
                T value = m_buffer.at(current_head);  // Copy the element to return
                m_head.store(increment(current_head), std::memory_order_release);  // Move head forward
                //--------------------------
                return value;
                //--------------------------
            }// end std::optional<T> get_top_pop(void)
            //--------------------------
            bool pop_front(void) override {
                //--------------------------
                size_t current_head = m_head.load(std::memory_order_relaxed);
                //--------------------------
                if (m_count.load(std::memory_order_acquire) == 0) {
                    return false;  // Buffer is empty, nothing to pop
                }// end if (size.load(std::memory_order_acquire) == 0)
                //--------------------------
                size_t next_head = increment(current_head);
                //--------------------------
                m_buffer[current_head].~T();  // Call destructor
                //--------------------------
                m_head.store(next_head, std::memory_order_release);
                m_count.fetch_sub(1, std::memory_order_release);
                //--------------------------
                return true;
                //--------------------------
            }// end bool pop_front(void)
            //--------------------------
            bool is_empty(void) const override {
                //--------------------------
                return m_count.load(std::memory_order_acquire) == 0;
                //--------------------------
            }// end bool is_empty(void) const
            //--------------------------
            size_t get_size(void) const override {
                //--------------------------
                return m_count.load(std::memory_order_acquire);
                //--------------------------
            }// end size_t get_size(void) const
            //--------------------------
            void clear(void) override {
                //--------------------------
                while (!is_empty()) {
                    pop_front();
                }// end while (!is_empty())
                //--------------------------
            }// end void clear(void)
            //--------------------------
            std::optional<std::span<T>> get_span(void) override {
                //--------------------------
                const size_t head   = m_head.load(std::memory_order_acquire);
                const size_t tail   = m_tail.load(std::memory_order_acquire);
                const size_t count  = m_count.load(std::memory_order_acquire);
                //--------------------------
                if (head <= tail) {
                    return std::span<T>(m_buffer.data() + head, count);
                }// end if (head <= tail)
                //--------------------------
                // Data wraps around, contiguous span is not possible
                //--------------------------
                return std::nullopt;
                //--------------------------
            }// end std::optional<std::span<T>> get_span()
            //--------------------------
            size_t increment(const size_t& value) const {
                return (value + 1) % N;
            }// end size_t increment(const size_t& value) const
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            std::atomic<size_t> m_head{0}, m_tail{0}, m_count{0};
            std::array<T, N> m_buffer;
        //--------------------------------------------------------------
    };//end class CircularBufferFixed : protected CircularBufferBased
    //--------------------------------------------------------------
}// end namespace CircularBuffer