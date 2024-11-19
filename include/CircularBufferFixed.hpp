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
#if __has_include(<execution>)
    #if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
        #include <execution>
    #endif
#endif
//--------------------------------------------------------------
/**
 * @namespace CircularBuffer
 * @brief Namespace for the CircularBuffer implementation.
 */
namespace CircularBuffer {
    //--------------------------------------------------------------
    /**
     * @class CircularBufferFixed
     * @brief A fixed-size Lock-free circular buffer implementation.
     *
     * This class provides a fixed-size circular buffer for any data type T. It is thread-safe and 
     * includes additional statistical methods for arithmetic types.
     *
     * @tparam T Type of elements stored in the buffer.
     * @tparam N Size of the buffer.
     *
     * @example usage:
     * @code
     * #include "CircularBuffer.hpp"
     *
     * int main() {
     *     CircularBuffer::CircularBufferFixed<int, 5> buffer;
     *     
     *     buffer.push(1);
     *     buffer.push(2);
     *     buffer.push(3);
     *     
     *     auto top = buffer.top();
     *     if (top) {
     *         std::cout << "Top element: " << *top << std::endl;
     *     }
     *     
     *     buffer.pop();
     *     
     *     std::cout << "Buffer size after pop: " << buffer.size() << std::endl;
     *     
     *     return 0;
     * }
     * @endcode
     */
    template <typename T, size_t N>
    class CircularBufferFixed {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            /**
             * @brief Default constructor for non-arithmetic types.
             *
             * Initializes the buffer with head, tail, and count set to 0.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<std::string, 3> buffer;
             *
             * buffer.push("Hello");
             * buffer.push("World");
             *
             * auto top = buffer.top();
             * if (top) {
             *     std::cout << "Top element: " << *top << std::endl;
             * }
             * @endcode
             */
            template <typename U = T, std::enable_if_t<!std::is_arithmetic<U>::value, int> = 0>
            CircularBufferFixed(void) : m_head(0), m_tail(0), m_count(0) {
                //--------------------------
            }// end CircularBufferFixed(void)
            //--------------------------
            /**
             * @brief Default constructor for arithmetic types.
             *
             * Initializes the buffer with head, tail, count, sum, and sum of squares set to 0.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             *
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             *
             * auto sum = buffer.sum();
             * if (sum) {
             *     std::cout << "Sum of elements: " << *sum << std::endl;
             * }
             *
             * auto mean = buffer.mean();
             * if (mean) {
             *     std::cout << "Mean of elements: " << *mean << std::endl;
             * }
             * @endcode
             */
            template <typename U = T, std::enable_if_t<std::is_arithmetic<U>::value, int> = 0>
            CircularBufferFixed(void) : m_head(0), m_tail(0), m_count(0), m_sum(0), m_sum_squares(0){
                //--------------------------
            }// end CircularBufferFixed(void)
            //--------------------------
            /**
             * @brief Copy constructor.
             *
             * Initializes the buffer with the values from another CircularBufferFixed instance. This constructor
             * performs a deep copy of the other buffer's state, including the head, tail, count, and buffer contents.
             * If the data type T is arithmetic, the sum and sum of squares are also copied.
             *
             * @param other The CircularBufferFixed instance to copy from.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer1;
             * buffer1.push(1);
             * buffer1.push(2);
             * buffer1.push(3);
             *
             * CircularBuffer::CircularBufferFixed<int, 5> buffer2 = buffer1;
             *
             * auto top = buffer2.top();
             * if (top) {
             *     std::cout << "Top element in buffer2: " << *top << std::endl;
             * }
             * @endcode
             */
            CircularBufferFixed(const CircularBufferFixed& other) : m_head(other.m_head.load()), 
                                                                    m_tail(other.m_tail.load()), 
                                                                    m_count(other.m_count.load()), 
                                                                    m_buffer(other.m_buffer){
                //--------------------------
                if constexpr (std::is_arithmetic_v<T>) {
                    m_sum           = other.m_sum.load();
                    m_sum_squares   = other.m_sum_squares.load();
                }//end if constexpr (std::is_arithmetic_v<T>)
                //--------------------------
            }// end CircularBufferFixed(const CircularBufferFixed& other)
            //--------------------------
            /**
             * @brief Copy assignment operator.
             *
             * Copies the values from another CircularBufferFixed instance. This operator performs a deep copy of the other
             * buffer's state, including the head, tail, count, and buffer contents. If the data type T is arithmetic, the sum
             * and sum of squares are also copied.
             *
             * @param other The CircularBufferFixed instance to copy from.
             * @return A reference to this CircularBufferFixed instance.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer1;
             * buffer1.push(1);
             * buffer1.push(2);
             * buffer1.push(3);
             *
             * CircularBuffer::CircularBufferFixed<int, 5> buffer2;
             * buffer2 = buffer1;
             *
             * auto top = buffer2.top();
             * if (top) {
             *     std::cout << "Top element in buffer2: " << *top << std::endl;
             * }
             * @endcode
             */
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
                }//end if constexpr (std::is_arithmetic_v<T>)
                //--------------------------
                return *this;
            }// end CircularBufferFixed& operator=(const CircularBufferFixed& other)
            //--------------------------
            /**
             * @brief Move constructor.
             *
             * Initializes the buffer by moving the values from another CircularBufferFixed instance. This constructor
             * transfers ownership of the other buffer's state to the new buffer, leaving the other buffer in a valid but
             * unspecified state.
             *
             * @param other The CircularBufferFixed instance to move from.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer1;
             * buffer1.push(1);
             * buffer1.push(2);
             * buffer1.push(3);
             *
             * CircularBuffer::CircularBufferFixed<int, 5> buffer2 = std::move(buffer1);
             *
             * auto top = buffer2.top();
             * if (top) {
             *     std::cout << "Top element in buffer2: " << *top << std::endl;
             * }
             * @endcode
             */
            CircularBufferFixed(CircularBufferFixed&&)                      = default;
            //--------------------------
            /**
             * @brief Move assignment operator.
             *
             * Moves the values from another CircularBufferFixed instance to this instance. This operator transfers ownership
             * of the other buffer's state to this buffer, leaving the other buffer in a valid but unspecified state.
             *
             * @param other The CircularBufferFixed instance to move from.
             * @return A reference to this CircularBufferFixed instance.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer1;
             * buffer1.push(1);
             * buffer1.push(2);
             * buffer1.push(3);
             *
             * CircularBuffer::CircularBufferFixed<int, 5> buffer2;
             * buffer2 = std::move(buffer1);
             *
             * auto top = buffer2.top();
             * if (top) {
             *     std::cout << "Top element in buffer2: " << *top << std::endl;
             * }
             * @endcode
             */
            CircularBufferFixed& operator=(CircularBufferFixed&&)           = default;
            //--------------------------
            ~CircularBufferFixed(void) = default;
            //--------------------------
            /**
             * @brief Pushes an item to the back of the buffer.
             *
             * This method adds a new item to the back of the buffer. If the buffer is full, it will 
             * remove the item from the front to make space for the new item.
             *
             * @param item The item to push.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * 
             * auto top = buffer.top();
             * if (top) {
             *     std::cout << "Top element: " << *top << std::endl;
             * }
             * @endcode
             */
            void push(const T& item) {
                push_back(item);
            }// end void push(const T& item) 
            //--------------------------
            /**
             * @brief Pushes an item to the back of the buffer using move semantics.
             *
             * This method adds a new item to the back of the buffer, moving the item instead of copying it. 
             * If the buffer is full, it will remove the item from the front to make space for the new item.
             *
             * @param item The item to push.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<std::string, 5> buffer;
             * buffer.push("Hello");
             * buffer.push("World");
             * 
             * std::string data = "Example";
             * buffer.push(std::move(data));
             * 
             * auto top = buffer.top();
             * if (top) {
             *     std::cout << "Top element: " << *top << std::endl;
             * }
             * @endcode
             */
            void push(T&& item) {
                push_back(std::move(item));
            }// end void push(T&& item)
            //--------------------------
            /**
             * @brief Emplaces an item to the back of the buffer.
             *
             * This method constructs a new item in place at the back of the buffer. If the buffer is full, 
             * it will remove the item from the front to make space for the new item.
             *
             * @tparam Args Types of arguments for constructing the item.
             * @param args Arguments for constructing the item.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<std::pair<int, std::string>, 5> buffer;
             * buffer.emplace(1, "One");
             * buffer.emplace(2, "Two");
             * buffer.emplace(3, "Three");
             * 
             * auto top = buffer.top();
             * if (top) {
             *     std::cout << "Top element: (" << top->first << ", " << top->second << ")" << std::endl;
             * }
             * @endcode
             */
            template <typename... Args>
            void emplace(Args&&... args) {
                emplace_back(std::forward<Args>(args)...);
            }// end void emplace(Args&&... args)
            //--------------------------
            /**
             * @brief Pops an item from the front of the buffer.
             *
             * This method removes the item at the front of the buffer. If the buffer is empty, it returns false.
             * If the buffer contains items, it removes the front item and returns true.
             *
             * @return True if an item was popped, false if the buffer was empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * 
             * while (buffer.pop()) {
             *     std::cout << "Popped an item from the buffer" << std::endl;
             * }
             * 
             * if (!buffer.pop()) {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            bool pop(void) {
                return pop_front();
            }// end void pop(void)
            //--------------------------
            /**
             * @brief Gets the top item from the buffer without removing it.
             *
             * This method returns the item at the front of the buffer without removing it. If the buffer is empty,
             * it returns std::nullopt.
             *
             * @return The top item, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * 
             * auto top = buffer.top();
             * if (top) {
             *     std::cout << "Top element: " << *top << std::endl;
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            std::optional<T> top(void) const {
                return get_top();
            }//end void top(void)
            //--------------------------
            /**
             * @brief Gets the last item from the buffer without removing it.
             *
             * This method returns the item at the back of the buffer without removing it. If the buffer is empty,
             * it returns std::nullopt.
             *
             * @return The last item, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * 
             * auto last = buffer.last();
             * if (last) {
             *     std::cout << "Last element: " << *last << std::endl;
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            std::optional<T> last(void) const{
                return get_last();
            }//end void last(void) const
            //--------------------------
            /**
             * @brief Gets and pops the top item from the buffer.
             *
             * This method returns the item at the front of the buffer and removes it. If the buffer is empty,
             * it returns std::nullopt.
             *
             * @return The top item, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * 
             * auto top = buffer.top_pop();
             * if (top) {
             *     std::cout << "Top element: " << *top << std::endl;
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * 
             * top = buffer.top_pop();
             * if (top) {
             *     std::cout << "Top element: " << *top << std::endl;
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            std::optional<T> top_pop(void){
                return get_top_pop();
            }// end std::optional<T> pop(void)
            //--------------------------
            /**
             * @brief Checks if the buffer is empty.
             *
             * This method returns true if the buffer is empty (i.e., it contains no elements), and false otherwise.
             *
             * @return True if the buffer is empty, false otherwise.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * 
             * if (buffer.empty()) {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * 
             * buffer.push(1);
             * 
             * if (!buffer.empty()) {
             *     std::cout << "Buffer is not empty" << std::endl;
             * }
             * @endcode
             */
            bool empty(void) const {   
                return is_empty();
            }// end bool empty(void) const
            //--------------------------
            /**
             * @brief Gets the number of items in the buffer.
             *
             * This method returns the current number of elements stored in the buffer.
             *
             * @return The number of items in the buffer.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * 
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * 
             * std::cout << "Buffer size: " << buffer.size() << std::endl; // Output: Buffer size: 3
             * @endcode
             */
            size_t size(void) const {   
                return get_size();
            }// end size_t size(void) const
            //--------------------------
            /**
             * @brief Clears the buffer.
             *
             * This method removes all elements from the buffer, effectively resetting it to its initial empty state.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * 
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * 
             * std::cout << "Buffer size before reset: " << buffer.size() << std::endl; // Output: Buffer size: 3
             * 
             * buffer.reset();
             * 
             * std::cout << "Buffer size after reset: " << buffer.size() << std::endl; // Output: Buffer size: 0
             * @endcode
             */
            void reset(void){
                clear();
            }// end void reset(void)
            //--------------------------
            /**
             * @brief Calculates the sum of the elements in the buffer.
             *
             * This method returns the sum of all elements currently stored in the buffer. This function is only available
             * for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * @return The sum of the elements, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * 
             * auto sum = buffer.sum();
             * if (sum) {
             *     std::cout << "Sum of elements: " << *sum << std::endl; // Output: Sum of elements: 6
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<U>> sum(void) const {
                return get_sum();
            }//end std::optional<T> sum(void) const
            //--------------------------
            /**
             * @brief Calculates the mean of the elements in the buffer.
             *
             * This method returns the mean (average) of all elements currently stored in the buffer. This function is only available
             * for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * The mean is calculated using the following equation:
             * \f[
             * \text{mean} = \frac{\sum_{i=1}^{n} x_i}{n}
             * \f]
             * where \f$\sum_{i=1}^{n} x_i\f$ is the sum of all elements in the buffer, and \f$n\f$ is the number of elements.
             *
             * @return The mean of the elements, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * 
             * auto mean = buffer.mean();
             * if (mean) {
             *     std::cout << "Mean of elements: " << *mean << std::endl; // Output: Mean of elements: 2
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> mean(void) const {
                return get_mean();
            }//end std::optional<T> mean(void) const
            //--------------------------
            /**
             * @brief Calculates the variance of the elements in the buffer.
             *
             * This method returns the variance of all elements currently stored in the buffer. This function is only available
             * for arithmetic types. If the buffer is empty or contains only one element, it returns std::nullopt.
             *
             * The variance is calculated using the following equation:
             * \f[
             * \text{variance} = \frac{1}{n-1} \sum_{i=1}^{n} (x_i - \mu)^2
             * \f]
             * where \f$n\f$ is the number of elements, \f$x_i\f$ is each element, and \f$\mu\f$ is the mean of the elements.
             *
             * @return The variance of the elements, or std::nullopt if the buffer is empty or contains only one element.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * buffer.push(4);
             * buffer.push(5);
             * 
             * auto variance = buffer.variance();
             * if (variance) {
             *     std::cout << "Variance of elements: " << *variance << std::endl;
             * } else {
             *     std::cout << "Buffer has less than two elements" << std::endl;
             * }
             * @endcode
             */
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> variance(void) const {
                return get_variance();
            }//end std::optional<T> variance(void) const
            //--------------------------
            /**
             * @brief Calculates the standard deviation of the elements in the buffer.
             *
             * This method returns the standard deviation of all elements currently stored in the buffer. This function is only available
             * for arithmetic types. If the buffer is empty or contains only one element, it returns std::nullopt.
             *
             * The standard deviation is calculated as the square root of the variance:
             * \f[
             * \text{standard deviation} = \sqrt{\text{variance}}
             * \f]
             *
             * @return The standard deviation of the elements, or std::nullopt if the buffer is empty or contains only one element.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * buffer.push(4);
             * buffer.push(5);
             * 
             * auto stddev = buffer.standard_deviation();
             * if (stddev) {
             *     std::cout << "Standard deviation of elements: " << *stddev << std::endl;
             * } else {
             *     std::cout << "Buffer has less than two elements" << std::endl;
             * }
             * @endcode
             */
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> standard_deviation(void) const {
                return get_standard_deviation();
            }//end std::optional<T> standard_deviation(void) const
            //--------------------------
            /**
             * @brief Finds the minimum element in the buffer.
             *
             * This method returns the minimum element currently stored in the buffer. This function is only available
             * for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * @return The minimum element, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * buffer.push(3);
             * buffer.push(1);
             * buffer.push(4);
             * buffer.push(1);
             * buffer.push(5);
             * 
             * auto min = buffer.minimum();
             * if (min) {
             *     std::cout << "Minimum element: " << *min << std::endl; // Output: Minimum element: 1
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<T>> minimum(void) const {
                return get_min();
            }//end std::optional<T> min(void) const
            //--------------------------
            /**
             * @brief Finds the maximum element in the buffer.
             *
             * This method returns the maximum element currently stored in the buffer. This function is only available
             * for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * @return The maximum element, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * buffer.push(3);
             * buffer.push(1);
             * buffer.push(4);
             * buffer.push(1);
             * buffer.push(5);
             * 
             * auto max = buffer.maximum();
             * if (max) {
             *     std::cout << "Maximum element: " << *max << std::endl; // Output: Maximum element: 5
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<T>> maximum(void) const {
                return get_max();
            }//end std::optional<T> max(void) const
            //--------------------------
            /**
             * @brief Returns a sorted copy of the buffer.
             *
             * This method returns a sorted copy of all elements currently stored in the buffer in ascending order. 
             * This function is only available for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * @return A sorted std::array of the elements, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * buffer.push(3);
             * buffer.push(1);
             * buffer.push(4);
             * buffer.push(1);
             * buffer.push(5);
             * 
             * auto sorted_buffer = buffer.sorted();
             * if (sorted_buffer) {
             *     std::cout << "Sorted elements: ";
             *     for (const auto& elem : *sorted_buffer) {
             *         std::cout << elem << " "; // Output: 1 1 3 4 5
             *     }
             *     std::cout << std::endl;
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<std::array<T, N>>> sorted(void) const {
                return get_sorted();
            }//end std::optional<std::array<T, N>> sorted(void) const
            //--------------------------
            /**
             * @brief Returns a reverse sorted copy of the buffer.
             *
             * This method returns a reverse sorted copy of all elements currently stored in the buffer in descending order.
             * This function is only available for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * @return A reverse sorted std::array of the elements, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * buffer.push(3);
             * buffer.push(1);
             * buffer.push(4);
             * buffer.push(1);
             * buffer.push(5);
             * 
             * auto reverse_sorted_buffer = buffer.reverse_sorted();
             * if (reverse_sorted_buffer) {
             *     std::cout << "Reverse sorted elements: ";
             *     for (const auto& elem : *reverse_sorted_buffer) {
             *         std::cout << elem << " "; // Output: 5 4 3 1 1
             *     }
             *     std::cout << std::endl;
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<std::array<T, N>>> reverse_sorted(void) const {
                return get_reverse_sorted();
            }//end std::optional<std::array<T, N>> reverse_sorted(void) const
            //--------------------------
            /**
             * @brief Calculates the median of the elements in the buffer.
             *
             * This method returns the median of all elements currently stored in the buffer. This function is only available
             * for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * The median is obtained by first copying the elements to a temporary buffer, using std::nth_element to partially sort
             * the buffer to find the median value. If the number of elements is odd, the middle value is returned. If the number of elements
             * is even, the average of the two middle values is returned.
             *
             * The time complexity of obtaining the median is O(n) on average, but O(n^2) in the worst case due to the nth_element algorithm.
             *
             * @return The median of the elements, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferFixed<int, 5> buffer;
             * buffer.push(3);
             * buffer.push(1);
             * buffer.push(4);
             * buffer.push(1);
             * buffer.push(5);
             * 
             * auto median = buffer.median();
             * if (median) {
             *     std::cout << "Median of elements: " << *median << std::endl; // Output: Median of elements: 3
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> median(void) const {
                return get_median();
            }//end std::optional<T> median(void) const
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
            std::tuple<size_t, size_t> insert(void) {
                //--------------------------
                size_t current_tail{0}, next_tail{0};
                //--------------------------
                do {
                    current_tail = m_tail.load(std::memory_order_relaxed);
                    //--------------------------
                    next_tail = increment(current_tail);
                    //--------------------------
                    if (is_full()) {  // Buffer is full
                        pop_front_unsafe();
                    }// end if (is_full())
                    //--------------------------
                } while (!m_tail.compare_exchange_weak(current_tail, next_tail, std::memory_order_release, std::memory_order_relaxed));
                //--------------------------
                return {current_tail, next_tail};
                //--------------------------
            } //end std::tuple<size_t, size_t> insert(void) const
            //--------------------------
            void push_back(const T& item)  {
                //--------------------------
                const auto [current_tail, next_tail] = insert(); 
                //--------------------------
                m_buffer.at(current_tail) = item;
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value){
                    atomic_add(item);
                }//end if constexpr (std::is_arithmetic<T>::value)
                //--------------------------
                m_tail.store(next_tail, std::memory_order_release);
                m_count.fetch_add(1, std::memory_order_release);
                //--------------------------
            }//end bool push_back(const T& item)
            //--------------------------
            void push_back(T&& item)  {
                //--------------------------
                const auto [current_tail, next_tail] = insert();
                //--------------------------
                m_buffer.at(current_tail) = std::move(item);
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value){
                    //--------------------------
                    atomic_add(m_buffer.at(current_tail));
                    //--------------------------
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
                size_t current_tail;
                std::tie(current_tail, std::ignore) = insert();
                //--------------------------
                new (&m_buffer[current_tail]) T(std::forward<Args>(args)...);  // Construct in-place
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value){
                    //--------------------------
                    atomic_add(m_buffer.at(current_tail));
                    //--------------------------
                }//end if constexpr (std::is_arithmetic<T>::value)
                //--------------------------
                m_count.fetch_add(1, std::memory_order_release);
                //--------------------------
            }// end void emplace_back(Args&&... args)
            //--------------------------
            std::optional<T> get_top(void) const {
                //--------------------------
                if (is_empty()) {
                    return std::nullopt;
                }// end if (is_empty())
                //--------------------------
                return m_buffer.at(m_head.load(std::memory_order_acquire));
                //--------------------------
            }//end std::optional<T> get_top(void) const
            //--------------------------
            std::optional<T> get_last(void) const {
                //--------------------------
                if (is_empty()) {
                    return std::nullopt;
                }// end if (is_empty())
                //--------------------------
                size_t _tail_position = m_tail.load(std::memory_order_acquire);
                _tail_position        = (_tail_position == 0) ? (N - 1) : (_tail_position - 1);
                //--------------------------
                return m_buffer.at(_tail_position);
                //--------------------------
            }//end std::optional<T> get_last(void) const
            //--------------------------
            std::optional<T> get_top_pop(void)  {
                //--------------------------
                size_t current_head{0}, next_head{0};
                //--------------------------
                do {
                    //--------------------------
                    current_head = m_head.load(std::memory_order_acquire);
                    //--------------------------
                    if (is_empty()) {
                        //--------------------------
                        return std::nullopt;
                        //--------------------------
                    }// end if (is_empty())
                    //--------------------------
                    next_head = increment(current_head);
                    //--------------------------
                } while (!m_head.compare_exchange_weak(current_head, next_head,
                                                    std::memory_order_release,
                                                    std::memory_order_relaxed));
                //--------------------------
                T value = m_buffer.at(current_head);
                //--------------------------
                m_buffer[current_head].~T();
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value) {
                    //--------------------------
                    atomic_sub(value);
                    //--------------------------
                }//end if constexpr (std::is_arithmetic<T>::value)
                //--------------------------
                m_count.fetch_sub(1, std::memory_order_relaxed);
                //--------------------------
                return value;
                //--------------------------
            }// end std::optional<T> get_top_pop(void)
            //--------------------------
            bool pop_front(void) {
                //--------------------------
                size_t current_head{0}, next_head{0};
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
                if constexpr (std::is_arithmetic<T>::value){
                    //--------------------------
                    atomic_sub(m_buffer.at(current_head));
                    //--------------------------
                }//end if constexpr (std::is_arithmetic<T>::value)
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
            //--------------------------
            size_t get_size(void) const  {
                //--------------------------
                return m_count.load(std::memory_order_acquire);
                //--------------------------
            }// end size_t get_size(void) const
            //--------------------------
            void clear(void)  {
                //--------------------------
                while (!is_empty()) {
                    //--------------------------
                    if(!pop_front()){
                        break;
                    }//end if(!pop_front())
                    //--------------------------
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
                return static_cast<double>(m_sum.load(std::memory_order_relaxed)) / m_count.load(std::memory_order_relaxed);
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
                const size_t count_ = m_count.load(std::memory_order_relaxed);
                //--------------------------
                if (count_ < 2) {
                    return std::nullopt;
                }//end if (count < 2)
                //--------------------------
                const double mean_ = get_mean().value_or(0.);
                //--------------------------
                return ((static_cast<double>(m_sum_squares.load(std::memory_order_relaxed)) / static_cast<double>(count_)) - std::pow(mean_, 2)) * 
                            (static_cast<double>(count_) / static_cast<double>(count_ - 1));
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
                return std::sqrt(get_variance().value_or(0.));
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
#if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
                return *std::min_element(std::execution::par, m_buffer.begin(), m_buffer.begin() + m_count.load(std::memory_order_relaxed));
#else
                return *std::min_element(m_buffer.begin(), m_buffer.begin() + m_count.load(std::memory_order_relaxed));
#endif
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
#if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
                return *std::max_element(std::execution::par, m_buffer.begin(), m_buffer.begin() + m_count.load(std::memory_order_relaxed));
#else
                return *std::max_element(m_buffer.begin(), m_buffer.begin() + m_count.load(std::memory_order_relaxed));
#endif
                //--------------------------
            }//end std::optional<T> get_max(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<std::array<T, N>>> get_sorted(void) const {
                //--------------------------
                if(is_empty()){
                    return std::nullopt;
                }//end if(is_empty())
                //--------------------------
                auto sorted_buffer = m_buffer;
#if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
                std::sort(std::execution::par, sorted_buffer.begin(), sorted_buffer.begin() + m_count.load(std::memory_order_relaxed));
#else
                std::sort(sorted_buffer.begin(), sorted_buffer.begin() + m_count.load(std::memory_order_relaxed));
#endif
                return sorted_buffer;
                //--------------------------
            }//end std::optional<std::array<T, N>> get_sorted(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<std::array<T, N>>> get_reverse_sorted(void) const {
                //--------------------------
                if(is_empty()){
                    return std::nullopt;
                }//end if(is_empty())
                //--------------------------
                auto sorted_buffer = m_buffer;
#if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
                std::sort(std::execution::par, sorted_buffer.begin(), sorted_buffer.begin() + m_count.load(std::memory_order_relaxed), std::greater<T>());
#else
                std::sort(sorted_buffer.begin(), sorted_buffer.begin() + m_count.load(std::memory_order_relaxed), std::greater<T>());
#endif
                return sorted_buffer;
                //--------------------------
            }//end std::optional<std::array<T, N>> get_reverse_sorted(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> get_median(void) const {
                //--------------------------
                if(is_empty()){
                    return std::nullopt;
                }//end if(is_empty())
                //--------------------------
                auto sorted_buffer          = m_buffer;
                const size_t count_         = m_count.load(std::memory_order_relaxed);
                const size_t half_count_    = count_ / 2UL;
                //--------------------------
                if (count_ == 1) {
                    return static_cast<double>(sorted_buffer.at(0));
                }//end if (count_ == 1)
                //--------------------------
#if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
                std::nth_element(std::execution::par, sorted_buffer.begin(), sorted_buffer.begin() + half_count_, sorted_buffer.end());
#else
                std::nth_element(sorted_buffer.begin(), sorted_buffer.begin() + half_count_, sorted_buffer.end());
#endif
                //--------------------------
                if (count_ % 2UL == 0) {
                    //--------------------------
                    const double median_1 = static_cast<double>(sorted_buffer.at(half_count_));
#if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
                    std::nth_element(std::execution::par, sorted_buffer.begin(), sorted_buffer.begin() + half_count_ - 1, sorted_buffer.end());
#else
                    std::nth_element(sorted_buffer.begin(), sorted_buffer.begin() + half_count_ - 1, sorted_buffer.end());
#endif
                    const double median_2 = static_cast<double>(sorted_buffer.at(half_count_ - 1));
                    //--------------------------
                    return (median_1 + median_2) / 2.;
                    //--------------------------
                }//end if (count_ % 2 == 0)
                //--------------------------
                return static_cast<double>(sorted_buffer.at(half_count_));
                //--------------------------
            }//end std::optional<T> get_median(void) const
            //--------------------------
            template <typename U = T, typename R>
            std::enable_if_t<std::is_floating_point<U>::value, void>
            atomic_float_add(std::atomic<U>& atomic, const R& value) {
                //--------------------------
                U current = atomic.load(std::memory_order_acquire);
                U desired;
                //--------------------------
                do {
                    desired = current + value;
                } while (!atomic.compare_exchange_weak(current, desired, std::memory_order_release, std::memory_order_relaxed));
                //--------------------------
            }//end void atomic_float_add(std::atomic<T>& atomic, const T& value)
            //--------------------------
            template <typename U = T, typename R>
            std::enable_if_t<std::is_floating_point<U>::value, void>
            atomic_float_sub(std::atomic<U>& atomic, const R& value) {
                //--------------------------
                U current = atomic.load(std::memory_order_acquire);
                U desired;
                //--------------------------
                do {
                    desired = current - value;
                } while (!atomic.compare_exchange_weak(current, desired, std::memory_order_release, std::memory_order_relaxed));
                //--------------------------
            }//end void atomic_float_sub(std::atomic<T>& atomic, const T& value)
            //--------------------------
            template <typename U = T, typename R>
            std::enable_if_t<std::is_arithmetic<U>::value, void> atomic_add(const R& value) {
                //--------------------------
                if constexpr (std::is_floating_point<T>::value){
                    atomic_float_add(m_sum, value);
                    atomic_float_add(m_sum_squares, std::pow(value, 2));
                } else {
                    m_sum.fetch_add(value, std::memory_order_relaxed);
                    m_sum_squares.fetch_add(std::pow(value, 2), std::memory_order_relaxed);
                }//end if constexpr (std::is_floating_point<T>::value)
                //--------------------------
            }//end void atomic_add(std::atomic<T>& atomic, const T& value)
            //--------------------------
            template <typename U = T, typename R>
            std::enable_if_t<std::is_arithmetic<U>::value, void> atomic_sub(const R& value) {
                //--------------------------
                if constexpr (std::is_floating_point<T>::value){
                    atomic_float_sub(m_sum, value);
                    atomic_float_sub(m_sum_squares, std::pow(value, 2));
                } else {
                    m_sum.fetch_sub(value, std::memory_order_relaxed);
                    m_sum_squares.fetch_sub(std::pow(value, 2), std::memory_order_relaxed);
                }//end if constexpr (std::is_floating_point<T>::value)
                //--------------------------
            }//end void atomic_sub(std::atomic<T>& atomic, const T& value)
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            std::atomic<size_t> m_head, m_tail, m_count;
            typename std::conditional<std::is_arithmetic<T>::value, std::atomic<T>, std::nullptr_t>::type m_sum, m_sum_squares;
            std::array<T, N> m_buffer;
            //--------------------------
            void pop_front_unsafe(void)  {
                //--------------------------
                size_t current_head{0}, next_head{0};
                //--------------------------
                do {
                    //--------------------------
                    current_head = m_head.load(std::memory_order_acquire);
                    //--------------------------
                    next_head = increment(current_head);
                    //--------------------------
                } while (!m_head.compare_exchange_weak(current_head, next_head,
                                                    std::memory_order_release,
                                                    std::memory_order_relaxed));
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value){
                    //--------------------------
                    atomic_sub(m_buffer.at(current_head));
                    //--------------------------
                }//end if constexpr (std::is_arithmetic<T>::value)
                //--------------------------
                m_buffer[current_head].~T();
                //--------------------------
                // Decrement the count of elements
                //--------------------------
                m_count.fetch_sub(1, std::memory_order_acq_rel); // Using acq_rel for decrement to ensure total order with increment operations
                //--------------------------
            }// end void pop_front_unsafe(void)
        //--------------------------------------------------------------
    };//end class CircularBufferFixed
    //--------------------------------------------------------------
}// end namespace CircularBuffer