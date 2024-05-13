#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <iostream>
#include <optional>
#include <array>
#include <atomic>
#include <thread>
#include <type_traits>
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <tuple>
//--------------------------------------------------------------
namespace CircularBuffer {
    //--------------------------------------------------------------
    template <typename T, size_t N>
    class CircularBufferFixed {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            template <typename U = T, std::enable_if_t<!std::is_arithmetic<U>::value, int> = 0>
            CircularBufferFixed(void) : m_head(0), m_tail(0), m_count(0) {
                //--------------------------
            }// end CircularBufferFixed(void)
            //--------------------------
            template <typename U = T, std::enable_if_t<std::is_arithmetic<U>::value, int> = 0>
            CircularBufferFixed(void) : m_head(0), m_tail(0), m_count(0), m_sum(0), m_sum_squares(0){
                //--------------------------
            }// end CircularBufferFixed(void)
            //--------------------------
            // template <typename U = T, std::enable_if_t<!std::is_arithmetic<U>::value, int> = 0>
            // CircularBufferFixed(const CircularBufferFixed& other) : m_head(other.m_head.load()), 
            //                                                         m_tail(other.m_tail.load()), 
            //                                                         m_count(other.m_count.load()), 
            //                                                         m_buffer(other.m_buffer) {
            //     //--------------------------
            // }// end CircularBufferFixed(const CircularBufferFixed& other)
            // //--------------------------
            // template <typename U = T, std::enable_if_t<std::is_arithmetic<U>::value, int> = 0>
            CircularBufferFixed(const CircularBufferFixed& other) : m_head(other.m_head.load()), 
                                                                    m_tail(other.m_tail.load()), 
                                                                    m_count(other.m_count.load()), 
                                                                    m_buffer(other.m_buffer){
                //--------------------------
                if constexpr (std::is_arithmetic_v<T>) {
                    m_sum           = other.m_sum.load();
                    m_sum_squares   = other.m_sum_squares.load();
                }
                //--------------------------
            }// end CircularBufferFixed(const CircularBufferFixed& other)
            //--------------------------
            CircularBufferFixed& operator=(const CircularBufferFixed& other) {
                //--------------------------
                if(this == &other){
                    return *this;
                }//end if(this == &other)
                //--------------------------
                m_head              = other.m_head.load();
                m_tail              = other.m_tail.load();
                m_count             = other.m_count.load();
                m_buffer            = other.m_buffer;
                //--------------------------
                if constexpr (std::is_arithmetic_v<T>) {
                    m_sum           = other.m_sum.load();
                    m_sum_squares   = other.m_sum_squares.load();
                }
                //--------------------------
                return *this;
            }// end CircularBufferFixed& operator=(const CircularBufferFixed& other)
            //--------------------------
            CircularBufferFixed(CircularBufferFixed&&)                      = default;
            CircularBufferFixed& operator=(CircularBufferFixed&&)           = default;
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
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<U>> sum(void) const {
                return get_sum();
            }//end std::optional<T> sum(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> mean(void) const {
                return get_mean();
            }//end std::optional<T> mean(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> variance(void) const {
                return get_variance();
            }//end std::optional<T> variance(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> standard_deviation(void) const {
                return get_standard_deviation();
            }//end std::optional<T> standard_deviation(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<T>> minimum(void) const {
                return get_min();
            }//end std::optional<T> min(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<T>> maximum(void) const {
                return get_max();
            }//end std::optional<T> max(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<std::tuple<T, T>>> minimum_maximum(void) const {
                return get_min_max();
            }//end std::optional<T> min_max(void) const
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
                        //--------------------------
                        if constexpr (std::is_arithmetic<T>::value){
                            //--------------------------
                            const T old_item_ = get_top_pop().value_or(0);
                            m_sum.store(m_sum.load(std::memory_order_relaxed) - old_item_, std::memory_order_relaxed);
                            m_sum_squares.store(m_sum_squares.load(std::memory_order_relaxed) - std::pow(old_item_, 2), std::memory_order_relaxed);
                            //--------------------------
                        } else {
                            //--------------------------
                            static_cast<void>(pop_front());  // Attempt to clear space
                            //--------------------------
                        }//end if constexpr (std::is_arithmetic<T>::value)
                        //--------------------------
                    }// end if (is_full())
                    //--------------------------
                    if (next_tail == m_head.load(std::memory_order_acquire)) {
                        std::this_thread::yield(); // buffer full, yield before retrying
                        continue;
                    }// end if (next_tail == m_head.load(std::memory_order_acquire))
                    //--------------------------
                } while (!m_tail.compare_exchange_weak(current_tail, next_tail, std::memory_order_release, std::memory_order_relaxed));
                //--------------------------
                m_buffer.at(current_tail) = item;
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value){
                    m_sum.store(m_sum.load(std::memory_order_relaxed) + item, std::memory_order_relaxed);
                    m_sum_squares.store(m_sum_squares.load(std::memory_order_relaxed) + std::pow(item, 2), std::memory_order_relaxed);
                }//end if constexpr (std::is_arithmetic<T>::value)
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
                        //--------------------------
                        if constexpr (std::is_arithmetic<T>::value){
                            //--------------------------
                            const T old_item_ = get_top_pop().value_or(0);
                            m_sum.store(m_sum.load(std::memory_order_relaxed) - old_item_, std::memory_order_relaxed);
                            m_sum_squares.store(m_sum_squares.load(std::memory_order_relaxed) - std::pow(old_item_, 2), std::memory_order_relaxed);
                            //--------------------------
                        } else {
                            //--------------------------
                            static_cast<void>(pop_front());  // Attempt to clear space
                            //--------------------------
                        }//end if constexpr (std::is_arithmetic<T>::value)
                        //--------------------------
                    }// end if (is_full())
                    //--------------------------
                    if (next_tail == m_head.load(std::memory_order_acquire)) {
                        std::this_thread::yield(); // buffer full, yield before retrying
                        continue;
                    }//end if (next_tail == m_head.load(std::memory_order_acquire))
                    //--------------------------
                } while (!m_tail.compare_exchange_weak(current_tail, next_tail, std::memory_order_release, std::memory_order_relaxed));
                //--------------------------
                m_buffer.at(current_tail) = std::move(item);
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value){
                    const T& current_item_ = m_buffer.at(current_tail);
                    m_sum.store(m_sum.load(std::memory_order_relaxed) + current_item_, std::memory_order_relaxed);
                    m_sum_squares.store(m_sum_squares.load(std::memory_order_relaxed) + std::pow(current_item_, 2), std::memory_order_relaxed);
                }//end if constexpr (std::is_arithmetic<T>::value)
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
                        //--------------------------
                        if constexpr (std::is_arithmetic<T>::value){
                            //--------------------------
                            const T old_item_ = get_top_pop().value_or(0);
                            m_sum.store(m_sum.load(std::memory_order_relaxed) - old_item_, std::memory_order_relaxed);
                            m_sum_squares.store(m_sum_squares.load(std::memory_order_relaxed) - std::pow(old_item_, 2), std::memory_order_relaxed);
                            //--------------------------
                        } else {
                            //--------------------------
                            static_cast<void>(pop_front());  // Attempt to clear space
                            //--------------------------
                        }//end if constexpr (std::is_arithmetic<T>::value)
                        //--------------------------
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
                if constexpr (std::is_arithmetic<T>::value){
                    const T& current_item_ = m_buffer.at(current_tail);
                    m_sum.store(m_sum.load(std::memory_order_relaxed) + current_item_, std::memory_order_relaxed);
                    m_sum_squares.store(m_sum_squares.load(std::memory_order_relaxed) + std::pow(current_item_, 2), std::memory_order_relaxed);
                }//end if constexpr (std::is_arithmetic<T>::value)
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
                return m_buffer.at(current_head);
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
                size_t current_head, next_head;
                //--------------------------
                do {
                    //--------------------------
                    current_head = m_head.load(std::memory_order_acquire);
                    //--------------------------
                    if (is_empty()) {
                        return false; // Buffer is empty, nothing to pop
                    }// end if (is_empty())
                    //--------------------------
                    next_head = increment(current_head);
                    //--------------------------
                } while (!m_head.compare_exchange_weak(current_head, next_head,
                                                    std::memory_order_release,
                                                    std::memory_order_relaxed));
                //--------------------------
                // Destroy the object at the old head, if it's a non-trivial type
                //--------------------------
                m_buffer[current_head].~T();
                //--------------------------
                // Decrement the count of elements
                //--------------------------
                m_count.fetch_sub(1, std::memory_order_acq_rel); // Using acq_rel for decrement to ensure total order with increment operations
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
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<U>> get_sum(void) const {
                //--------------------------
                if(is_empty()){
                    return std::nullopt;
                }//end if(is_empty())
                //--------------------------
                return m_sum.load(std::memory_order_relaxed);
                //--------------------------
            }//end std::optional<T> get_sum(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> get_mean(void) const {
                //--------------------------
                if(is_empty()){
                    return std::nullopt;
                }//end if(is_empty())
                //--------------------------
                return static_cast<double>(static_cast<double>(m_sum.load(std::memory_order_relaxed)) / m_count.load(std::memory_order_relaxed));
                //--------------------------
            }//end std::optional<T> get_mean(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> get_variance(void) const {
                //--------------------------
                if(is_empty()){
                    return std::nullopt;
                }//end if(is_empty())
                //--------------------------
                const double mean_ = get_mean().value_or(0);
                return (m_sum_squares.load(std::memory_order_relaxed) / m_count.load(std::memory_order_relaxed)) - std::pow(mean_, 2);
                //--------------------------
            }//end std::optional<T> get_variance(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> get_standard_deviation(void) const {
                //--------------------------
                if(is_empty()){
                    return std::nullopt;
                }//end if(is_empty())
                //--------------------------
                return std::sqrt(get_variance().value_or(0));
                //--------------------------
            }//end std::optional<T> get_standard_deviation(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<T>> get_min(void) const {
                //--------------------------
                if(is_empty()){
                    return std::nullopt;
                }//end if(is_empty())
                //--------------------------
                return *std::min_element(m_buffer.begin(), m_buffer.begin() + m_count.load(std::memory_order_relaxed));
                //--------------------------
            }//end std::optional<T> get_min(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<T>> get_max(void) const {
                //--------------------------
                if(is_empty()){
                    return std::nullopt;
                }//end if(is_empty())
                //--------------------------
                return *std::max_element(m_buffer.begin(), m_buffer.begin() + m_count.load(std::memory_order_relaxed));
                //--------------------------
            }//end std::optional<T> get_max(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<std::tuple<T, T>>> get_min_max(void) const {
                //--------------------------
                if(is_empty()){
                    return std::nullopt;
                }//end if(is_empty())
                //--------------------------
                const auto min_max = std::minmax_element(m_buffer.begin(), m_buffer.begin() + m_count.load(std::memory_order_relaxed));
                return {*min_max.first, *min_max.second};
                //--------------------------
            }//end std::optional<T> get_min_max(void) const
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            std::atomic<size_t> m_head, m_tail, m_count;
            std::array<T, N> m_buffer;
            typename std::conditional<std::is_arithmetic<T>::value, std::atomic<T>, std::nullptr_t>::type m_sum, m_sum_squares;
        //--------------------------------------------------------------
    };//end class CircularBufferFixed
    //--------------------------------------------------------------
}// end namespace CircularBuffer