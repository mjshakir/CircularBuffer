#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <iostream>
#include <optional>
#include <vector>
#include <deque>
#include <mutex>
#include <type_traits>
#include <cstddef>
#include <cmath>
#include <algorithm>
#if __has_include(<execution>)
    #if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
        #include <execution>
    #endif
#endif
//--------------------------------------------------------------
namespace CircularBuffer {
    //--------------------------------------------------------------
    template <typename T>
    class CircularBufferDynamic {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            template <typename U = T, std::enable_if_t<!std::is_arithmetic<U>::value, int> = 0>
            explicit CircularBufferDynamic(const size_t& size) : m_max_size(size) {
                //--------------------------
            }// end explicit CircularBuffer(size_t size)
            //--------------------------
            template <typename U = T, std::enable_if_t<std::is_arithmetic<U>::value, int> = 0>
            explicit CircularBufferDynamic(const size_t& size) : m_max_size(size), m_sum(0), m_sum_squares(0){
                //--------------------------
            }// end explicit CircularBuffer(size_t size)
            //--------------------------
            CircularBufferDynamic(const CircularBufferDynamic& other) : m_max_size(other.m_max_size),
                                                                        m_buffer(other.m_buffer) {
                //--------------------------
                std::scoped_lock lock(m_mutex, other.m_mutex);
                //--------------------------
                if constexpr (std::is_arithmetic_v<T>) {
                    m_sum           = other.m_sum;
                    m_sum_squares   = other.m_sum_squares;
                }// end if constexpr (std::is_arithmetic_v<T>)
                //--------------------------
            }// end CircularBufferDynamic(const CircularBufferDynamic& other)
            //--------------------------
            CircularBufferDynamic& operator=(const CircularBufferDynamic& other) {
                //--------------------------
                if(this == &other){
                    return *this;
                }//end if(this == &other)
                //--------------------------
                std::scoped_lock lock(m_mutex, other.m_mutex);
                //--------------------------
                m_max_size = other.m_max_size;
                m_buffer = other.m_buffer;
                //--------------------------
                if constexpr (std::is_arithmetic_v<T>) {
                    m_sum           = other.m_sum.load();
                    m_sum_squares   = other.m_sum_squares.load();
                }// end if constexpr (std::is_arithmetic_v<T>)
                //--------------------------
                return *this;
            }// end CircularBufferDynamic& operator=(const CircularBufferDynamic& other)
            //--------------------------
            CircularBufferDynamic(CircularBufferDynamic&& other) noexcept : m_max_size(other.m_max_size),
                                                                            m_buffer(std::move(other.m_buffer)){
                //--------------------------
                std::scoped_lock lock(m_mutex, other.m_mutex);
                //--------------------------
                if constexpr (std::is_arithmetic_v<T>) {
                    m_sum           = std::move(other.m_sum);
                    m_sum_squares   = std::move(other.m_sum_squares);
                }// end if constexpr (std::is_arithmetic_v<T>)
                //--------------------------
            }// end CircularBufferDynamic(CircularBufferDynamic&& other)
            //--------------------------
            CircularBufferDynamic& operator=(CircularBufferDynamic&& other) noexcept {
                //--------------------------
                if(this == &other){
                    return *this;
                }//end if(this == &other)
                //--------------------------
                std::scoped_lock lock(m_mutex, other.m_mutex);
                //--------------------------
                m_max_size = other.m_max_size;
                m_buffer = std::move(other.m_buffer);
                //--------------------------
                if constexpr (std::is_arithmetic_v<T>) {
                    m_sum           = std::move(other.m_sum);
                    m_sum_squares   = std::move(other.m_sum_squares);
                }// end if constexpr (std::is_arithmetic_v<T>)
                //--------------------------
                return *this;
            }// end CircularBufferDynamic& operator=(const CircularBufferDynamic& other)
            //--------------------------
            CircularBufferDynamic(void) = delete;
            //--------------------------
            ~CircularBufferDynamic(void) = default;
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
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
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
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                return get_sum();
                //--------------------------
            }// end std::optional<T> sum(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> mean(void) const {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                return get_mean();
                //--------------------------
            }// end std::optional<T> mean(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> variance(void) const {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                return get_variance();
                //--------------------------
            }// end std::optional<T> variance(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> standard_deviation(void) const {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                return get_standard_deviation();
                //--------------------------
            }// end std::optional<T> standard_deviation(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<U>> minimum(void) const {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                return get_min();
                //--------------------------
            }// end std::optional<T> min(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<U>> maximum(void) const {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                return get_max();
                //--------------------------
            }// end std::optional<T> max(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<std::vector<U>>> sorted(void) const {
                //--------------------------
                return get_sorted();
                //--------------------------
            }// end std::optional<std::vector<T>> sorted(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<std::vector<U>>> reverse_sorted(void) const {
                //--------------------------
                return get_reverse_sorted();
                //--------------------------
            }// end std::optional<std::vector<T>> reverse_sorted(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> median(void) const {
                //--------------------------
                return get_median();
                //--------------------------
            }// end std::optional<T> median(void) const
            //--------------------------
            typename std::deque<T>::iterator begin(void) {
                return m_buffer.begin();
            }//end typename std::deque<T>::iterator begin(void)
            //--------------------------
            typename std::deque<T>::iterator end(void) {
                return m_buffer.end();
            }// end typename std::deque<T>::iterator
            //--------------------------
            typename std::deque<T>::const_iterator begin(void) const {
                return m_buffer.begin();
            }// end typename std::deque<T>::const_iterator begin(void) const
            //--------------------------
            typename std::deque<T>::const_iterator end(void) const {
                return m_buffer.end();
            }//end typename std::deque<T>::const_iterator end(void) const
            //--------------------------
            typename std::deque<T>::const_iterator cbegin(void) const {
                return m_buffer.cbegin();
            }//end typename std::deque<T>::const_iterator cbegin(void) const
            //--------------------------
            typename std::deque<T>::const_iterator cend(void) const {
                return m_buffer.cend();
            }//end typename std::deque<T>::const_iterator cend(void) const
            //--------------------------
            typename std::deque<T>::reverse_iterator rbegin(void) {
                return m_buffer.rbegin();
            }//end typename std::deque<T>::reverse_iterator rbegin(void)
            //--------------------------
            typename std::deque<T>::reverse_iterator rend(void) {
                return m_buffer.rend();
            }//end typename std::deque<T>::reverse_iterator rend(void)
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            void push_back(const T& item)  {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.size() == m_max_size) {
                    //--------------------------
                    static_cast<void>(pop_front());
                    //--------------------------
                }// end if (m_buffer.size() == m_max_size)
                //--------------------------
                m_buffer.push_back(item);
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value){
                    //--------------------------
                    m_sum += item;
                    m_sum_squares += std::pow(item, 2);
                    //--------------------------
                }// end if constexpr (std::is_arithmetic<T>::value)
                //--------------------------
            }// end void push_back(const T& item)
            //--------------------------
            void push_back(T&& item)  {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.size() == m_max_size) {
                    //--------------------------
                    static_cast<void>(pop_front());
                    //--------------------------
                }// end if (m_buffer.size() == m_max_size)
                //--------------------------
                m_buffer.push_back(std::move(item));
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value){
                    //--------------------------
                    const T& current_item_ = m_buffer.back();
                    m_sum += current_item_;
                    m_sum_squares += std::pow(current_item_, 2);
                    //--------------------------
                }// end if constexpr (std::is_arithmetic<T>::value)
                //--------------------------
            }// end void push_back(T&& item)
            //--------------------------
            template <typename... Args>
            void emplace_back(Args&&... args)  {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.size() == m_max_size) {
                    //--------------------------
                    static_cast<void>(pop_front());
                    //--------------------------
                }//end if (m_buffer.size() == m_max_size)
                //--------------------------
                m_buffer.emplace_back(std::forward<Args>(args)...);
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value){
                    //--------------------------
                    const T& current_item_ = m_buffer.back();
                    m_sum += current_item_;
                    m_sum_squares += std::pow(current_item_, 2);
                    //--------------------------
                }// end if constexpr (std::is_arithmetic<T>::value)
                //--------------------------
            }// end void emplace_back(Args&&... args)
            //--------------------------
            std::optional<T> get_top_pop(void)  {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
                T value = m_buffer.front();
                static_cast<void>(m_buffer.pop_front());
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value){
                    //--------------------------
                    m_sum -= value;
                    m_sum_squares -= std::pow(value, 2);
                    //--------------------------
                }// end if constexpr (std::is_arithmetic<T>::value)
                //--------------------------
                return value;
                //--------------------------
            }// end std::optional<T> get_top_pop(void)
            //--------------------------
            std::optional<T> get_top(void)  {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
                return m_buffer.front();
                //--------------------------
            }// end T get_top(void)
            //--------------------------
            bool pop_front(void)  {
                //--------------------------
                if (m_buffer.empty()) {
                    return false;
                }// end if (m_buffer.empty())
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value){
                    //--------------------------
                    const T& front_ = m_buffer.front();
                    m_sum -= front_;
                    m_sum_squares -= std::pow(front_, 2);
                    //--------------------------
                }// end if constexpr (std::is_arithmetic<T>::value)
                //--------------------------
                m_buffer.pop_front();
                //--------------------------
                return true;
                //--------------------------
            }// end bool pop_front(void)
            //--------------------------
            bool is_empty(void) const  {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                return m_buffer.empty();
                //--------------------------
            }// end bool is_empty(void) const
            //--------------------------
            size_t get_size(void) const  {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                return m_buffer.size();
                //--------------------------
            }// end size_t get_size(void) const
            //--------------------------
            void clear(void)  {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                m_buffer.clear();
                //--------------------------
            }// end void clear(void)
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<U>> get_sum(void) const {
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
                return m_sum;
                //--------------------------
            }// end std::optional<T> get_sum(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> get_mean(void) const {
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
                return static_cast<double>(static_cast<double>(m_sum) / m_buffer.size());
                //--------------------------
            }// end std::optional<T> get_mean(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> get_variance(void) const {
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
                const size_t size_ = m_buffer.size();
                //--------------------------
                if (size_ < 2) {
                    return std::nullopt;
                }//end if (size_ < 2)
                //--------------------------
                const double mean_ = get_mean().value_or(0.);
                //--------------------------
                return ((static_cast<double>(m_sum_squares) / static_cast<double>(size_)) - std::pow(mean_, 2)) * 
                        (static_cast<double>(size_) / static_cast<double>(size_ - 1));
                //--------------------------
            }// end std::optional<T> get_variance(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> get_standard_deviation(void) const {
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
                return std::sqrt(get_variance().value_or(0));
                //--------------------------
            }// end std::optional<T> get_standard_deviation(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<U>> get_min(void) const {
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
#if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
                return *std::min_element(std::execution::par, m_buffer.begin(), m_buffer.end());
#else
                return *std::min_element(m_buffer.begin(), m_buffer.end());
#endif
                //--------------------------
            }// end std::optional<T> get_min(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<U>> get_max(void) const {
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
#if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
                return *std::max_element(std::execution::par, m_buffer.begin(), m_buffer.end());
#else
                return *std::max_element(m_buffer.begin(), m_buffer.end());
#endif
                //--------------------------
            }// end std::optional<T> get_max(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<std::vector<U>>> get_sorted(void) const {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
                std::vector<U> sorted_buffer(m_buffer.begin(), m_buffer.end());
                //--------------------------
#if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
                std::sort(std::execution::par, sorted_buffer.begin(), sorted_buffer.end());
#else
                std::sort(sorted_buffer.begin(), sorted_buffer.end());
#endif
                return sorted_buffer;
                //--------------------------
            }// end std::optional<std::vector<T>> get_sorted(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<std::vector<U>>> get_reverse_sorted(void) const {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
                std::vector<U> sorted_buffer(m_buffer.begin(), m_buffer.end());
                //--------------------------
#if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
                std::sort(std::execution::par, sorted_buffer.rbegin(), sorted_buffer.rend());
#else
                std::sort(sorted_buffer.rbegin(), sorted_buffer.rend());
#endif
                return sorted_buffer;
                //--------------------------
            }// end std::optional<std::vector<T>> get_reverse_sorted(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> get_median(void) const {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
                if (m_buffer.size() == 1) {
                    return static_cast<double>(m_buffer.front());
                }// end if (m_buffer.size() == 1)
                //--------------------------
                const size_t size_      = m_buffer.size();
                const size_t half_size_ = size_ / 2UL;
                //--------------------------
                if (size_ % 2UL == 0) {
                    //--------------------------
                    // For even number of elements, need to find two middle elements
                    //--------------------------
                    std::vector<T> temp(half_size_ + 1);
                    //--------------------------
#if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
                    std::partial_sort_copy(std::execution::par, m_buffer.begin(), m_buffer.end(), temp.begin(), temp.end());
#else
                    std::partial_sort_copy(m_buffer.begin(), m_buffer.end(), temp.begin(), temp.end());
#endif
                    //--------------------------
                    const double median_1 = static_cast<double>(temp.at(half_size_));
                    const double median_2 = static_cast<double>(temp.at(half_size_ - 1));
                    //--------------------------
                    return (median_1 + median_2) / 2.;
                    //--------------------------
                }// end if (size_ % 2UL == 0)
                //--------------------------
                // For odd number of elements, find the middle element
                //--------------------------
                std::vector<T> temp(half_size_ + 1);
                //--------------------------
#if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
                std::partial_sort_copy(std::execution::par, m_buffer.begin(), m_buffer.end(), temp.begin(), temp.end());
#else
                std::partial_sort_copy(m_buffer.begin(), m_buffer.end(), temp.begin(), temp.end());
#endif
                //--------------------------
                return static_cast<double>(temp.at(half_size_));
                //--------------------------
            }// end std::optional<T> get_median(void) const
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            const size_t m_max_size;
            std::deque<T> m_buffer;
            typename std::conditional<std::is_arithmetic<T>::value, T, std::nullptr_t>::type m_sum, m_sum_squares;
            mutable std::mutex m_mutex;
            //--------------------------------------------------------------
    };// end class CircularBufferDynamic
    //--------------------------------------------------------------
}// end namespace CircularBuffers
