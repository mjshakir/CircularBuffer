#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <iostream>
#include <optional>
#include <array>
#include <atomic>
#include <thread>
//--------------------------------------------------------------
namespace CircularBuffer {
    //--------------------------------------------------------------
    template <typename T, size_t N>
    class CircularBufferFixed {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            CircularBufferFixed(void) : m_head(0), m_tail(0), m_count(0) {
                //--------------------------
            }// end CircularBufferFixed(void)
            //--------------------------
            ~CircularBufferFixed(void) = default;
            //--------------------------
            void push(const T& item) {
                push_back(item);
            }// end void push(const T& item) 
            //--------------------------
            void push(T&& item) {
                push_back(std::move(item));
            }// end void push(T&& item)
            //--------------------------
            template <typename... Args>
            void emplace(Args&&... args) {
                emplace_back(std::forward<Args>(args)...);
            }// end void emplace(Args&&... args)
            //--------------------------
            bool pop(void) {
                return pop_front();
            }// end void pop(void)
            //--------------------------
            std::optional<T> top(void){
                return get_top();
            }//end void top(void)
            //--------------------------
            std::optional<T> top_pop(void){
                return get_top_pop();
            }// end std::optional<T> pop(void)
            //--------------------------
            bool empty(void) const {   
                return is_empty();
            }// end bool empty(void) const
            //--------------------------
            size_t size(void) const {   
                return get_size();
            }// end size_t size(void) const
            //--------------------------
            void reset(void){
                clear();
            }// end void reset(void)
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
            void push_back(const T& item)  {
                //--------------------------
                size_t current_tail{0}, next_tail{0};
                //--------------------------
                do {
                    current_tail = m_tail.load(std::memory_order_relaxed);
                    //--------------------------
                    next_tail = increment(current_tail);
                    //--------------------------
                    if (is_full()) {  // Buffer is full
                        static_cast<void>(pop_front());  // Attempt to clear space
                    }// end if (is_full())
                    //--------------------------
                    if (next_tail == m_head.load(std::memory_order_acquire)) {
                        std::this_thread::yield(); // buffer full, yield before retrying
                        continue;
                    }// end if (next_tail == m_head.load(std::memory_order_acquire))
                    //--------------------------
                } while (!m_tail.compare_exchange_weak(current_tail, next_tail, std::memory_order_release, std::memory_order_relaxed));
                //--------------------------
                m_buffer[current_tail] = item;
                //--------------------------
                m_tail.store(next_tail, std::memory_order_release);
                m_count.fetch_add(1, std::memory_order_release);
                //--------------------------
            }//end bool push_back(const T& item)
            //--------------------------
            void push_back(T&& item)  {
                //--------------------------
                size_t current_tail{0}, next_tail{0};
                //--------------------------
                do {
                    current_tail = m_tail.load(std::memory_order_relaxed);
                    //--------------------------
                    next_tail = increment(current_tail);
                    //--------------------------
                    if (is_full()) {  // Buffer is full
                        static_cast<void>(pop_front());  // Attempt to clear space
                    }// end if (is_full())
                    //--------------------------
                    if (next_tail == m_head.load(std::memory_order_acquire)) {
                        std::this_thread::yield(); // buffer full, yield before retrying
                        continue;
                    }//end if (next_tail == m_head.load(std::memory_order_acquire))
                    //--------------------------
                } while (!m_tail.compare_exchange_weak(current_tail, next_tail, std::memory_order_release, std::memory_order_relaxed));
                //--------------------------
                m_buffer[current_tail] = std::move(item);
                //--------------------------
                m_tail.store(next_tail, std::memory_order_release);
                m_count.fetch_add(1, std::memory_order_release);
                //--------------------------
            }//end bool push_back(const T& item)
            //--------------------------
            template <typename... Args>
            void emplace_back(Args&&... args)  {
                //--------------------------
                size_t current_tail{0}, next_tail{0};
                //--------------------------
                do {
                    current_tail = m_tail.load(std::memory_order_relaxed);
                    //--------------------------
                    next_tail = increment(current_tail);
                    //--------------------------
                    if (is_full()) {  // Buffer is full
                        static_cast<void>(pop_front());  // Attempt to clear space
                    }// end if (is_full())
                    //--------------------------
                    if (next_tail == m_head.load(std::memory_order_acquire)) {
                        std::this_thread::yield(); // buffer full, yield before retrying
                        continue;
                    }//end if (next_tail == m_head.load(std::memory_order_acquire))
                    //--------------------------
                } while (!m_tail.compare_exchange_weak(current_tail, next_tail, std::memory_order_release, std::memory_order_relaxed));
                //--------------------------
                new (&m_buffer[current_tail]) T(std::forward<Args>(args)...);  // Construct in-place
                //--------------------------
                m_count.fetch_add(1, std::memory_order_release);
                //--------------------------
            }// end void emplace_back(Args&&... args)
            //--------------------------
            std::optional<T> get_top(void)  {
                //--------------------------
                if (is_empty()) {
                    return std::nullopt;
                }// end if (is_empty())
                //--------------------------
                size_t current_head = m_head.load(std::memory_order_acquire);
                //--------------------------
                return m_buffer[current_head];
                //--------------------------
            }//end std::optional<T> get_top(void)
            //--------------------------
            std::optional<T> get_top_pop(void)  {
                //--------------------------
                if (is_empty()) {
                    return std::nullopt;
                }// end if (is_empty())
                //--------------------------
                size_t current_head = m_head.load(std::memory_order_acquire);
                //--------------------------
                T value = m_buffer[current_head];
                //--------------------------
                current_head = increment(current_head);
                //--------------------------
                m_head.store(current_head, std::memory_order_release);
                m_count.fetch_sub(1, std::memory_order_relaxed); // Decrement count after pop
                //--------------------------
                return value;
                //--------------------------
            }// end std::optional<T> get_top_pop(void)
            //--------------------------
            bool pop_front(void)  {
                //--------------------------
                size_t current_head = m_head.load(std::memory_order_relaxed);
                //--------------------------
                if (is_empty()) {
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
            bool is_empty(void) const  {
                //--------------------------
                return m_count.load(std::memory_order_acquire) == 0;
                //--------------------------
            }// end bool is_empty(void) const
            //--------------------------
            bool is_full(void) const {
                return m_count.load(std::memory_order_acquire) == N;
            }// end bool is_full() const
            size_t get_size(void) const  {
                //--------------------------
                return m_count.load(std::memory_order_acquire);
                //--------------------------
            }// end size_t get_size(void) const
            //--------------------------
            void clear(void)  {
                //--------------------------
                while (!is_empty()) {
                    if(!pop_front()){
                        break;
                    }
                }// end while (!is_empty())
                //--------------------------
            }// end void clear(void)
            //--------------------------
            size_t increment(const size_t& value) const {
                return (value + 1) % N;
            }// end size_t increment(const size_t& value) const
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            std::atomic<size_t> m_head, m_tail, m_count;
            std::array<T, N> m_buffer;
        //--------------------------------------------------------------
    };//end class CircularBufferFixed
    //--------------------------------------------------------------
}// end namespace CircularBuffer