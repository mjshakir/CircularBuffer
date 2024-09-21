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
/**
 * @namespace CircularBuffer
 * @brief Namespace for the CircularBuffer implementation.
 */
namespace CircularBuffer {
    //--------------------------------------------------------------
    /**
     * @class CircularBufferDynamic
     * @brief A dynamically-sized circular buffer implementation.
     *
     * This class provides a dynamically-sized circular buffer for any data type T. It is thread-safe and 
     * includes additional statistical methods for arithmetic types.
     *
     * @tparam T Type of elements stored in the buffer.
     *
     * @example usage:
     * @code
     * #include <iostream>
     * #include "CircularBufferDynamic.hpp"
     * 
     * int main() {
     *     // Example with non-arithmetic type
     *     CircularBuffer::CircularBufferDynamic<std::string> stringBuffer(3);
     *     stringBuffer.push("Hello");
     *     stringBuffer.push("World");
     *     stringBuffer.push("!");
     * 
     *     auto topString = stringBuffer.top();
     *     if (topString) {
     *         std::cout << "Top element: " << *topString << std::endl; // Output: Top element: Hello
     *     }
     * 
     *     stringBuffer.pop();
     *     topString = stringBuffer.top();
     *     if (topString) {
     *         std::cout << "Top element after pop: " << *topString << std::endl; // Output: Top element after pop: World
     *     }
     * 
     *     // Example with arithmetic type
     *     CircularBuffer::CircularBufferDynamic<int> intBuffer(3);
     *     intBuffer.push(1);
     *     intBuffer.push(2);
     *     intBuffer.push(3);
     * 
     *     auto sum = intBuffer.sum();
     *     if (sum) {
     *         std::cout << "Sum of elements: " << *sum << std::endl; // Output: Sum of elements: 6
     *     }
     * 
     *     auto mean = intBuffer.mean();
     *     if (mean) {
     *         std::cout << "Mean of elements: " << *mean << std::endl; // Output: Mean of elements: 2
     *     }
     * 
     *     return 0;
     * }
     * @endcode
     */
    template <typename T>
    class CircularBufferDynamic {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            /**
             * @brief Constructs a CircularBufferDynamic with a specified size for non-arithmetic types.
             *
             * Initializes the buffer with a maximum size and prepares it for storing elements. This constructor
             * is only available for non-arithmetic types.
             *
             * @param size The maximum size of the buffer.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferDynamic<std::string> buffer(5);
             * buffer.push("Hello");
             * buffer.push("World");
             * @endcode
             */
            template <typename U = T, std::enable_if_t<!std::is_arithmetic<U>::value, int> = 0>
            explicit CircularBufferDynamic(const size_t& size) : m_max_size(size) {
                //--------------------------
            }// end explicit CircularBuffer(size_t size)
            //--------------------------
            /**
             * @brief Constructs a CircularBufferDynamic with a specified size for arithmetic types.
             *
             * Initializes the buffer with a maximum size and prepares it for storing elements. This constructor
             * is only available for arithmetic types. It also initializes the sum and sum of squares to zero.
             *
             * @param size The maximum size of the buffer.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferDynamic<int> buffer(5);
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * @endcode
             */
            template <typename U = T, std::enable_if_t<std::is_arithmetic<U>::value, int> = 0>
            explicit CircularBufferDynamic(const size_t& size) : m_max_size(size), m_sum(0), m_sum_squares(0){
                //--------------------------
            }// end explicit CircularBuffer(size_t size)
            //--------------------------
            /**
             * @brief Copy constructor.
             *
             * Initializes the buffer with the values from another CircularBufferDynamic instance. This constructor
             * performs a deep copy of the other buffer's state, including the maximum size, buffer contents, and any
             * arithmetic sums if applicable.
             *
             * @param other The CircularBufferDynamic instance to copy from.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferDynamic<int> buffer1(5);
             * buffer1.push(1);
             * buffer1.push(2);
             * 
             * CircularBuffer::CircularBufferDynamic<int> buffer2 = buffer1;
             * @endcode
             */
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
            /**
             * @brief Copy assignment operator.
             *
             * Copies the values from another CircularBufferDynamic instance. This operator performs a deep copy of the other
             * buffer's state, including the maximum size, buffer contents, and any arithmetic sums if applicable.
             *
             * @param other The CircularBufferDynamic instance to copy from.
             * @return A reference to this CircularBufferDynamic instance.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferDynamic<int> buffer1(5);
             * buffer1.push(1);
             * buffer1.push(2);
             * 
             * CircularBuffer::CircularBufferDynamic<int> buffer2(5);
             * buffer2 = buffer1;
             * @endcode
             */
            CircularBufferDynamic& operator=(const CircularBufferDynamic& other) {
                //--------------------------
                if(this == &other){
                    return *this;
                }//end if(this == &other)
                //--------------------------
                std::scoped_lock lock(m_mutex, other.m_mutex);
                //--------------------------
                m_max_size  = other.m_max_size;
                m_buffer    = other.m_buffer;
                //--------------------------
                if constexpr (std::is_arithmetic_v<T>) {
                    m_sum           = other.m_sum.load();
                    m_sum_squares   = other.m_sum_squares.load();
                }// end if constexpr (std::is_arithmetic_v<T>)
                //--------------------------
                return *this;
            }// end CircularBufferDynamic& operator=(const CircularBufferDynamic& other)
            //--------------------------
            /**
             * @brief Move constructor.
             *
             * Initializes the buffer by moving the values from another CircularBufferDynamic instance. This constructor
             * transfers ownership of the other buffer's state to the new buffer, leaving the other buffer in a valid but
             * unspecified state.
             *
             * @param other The CircularBufferDynamic instance to move from.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferDynamic<int> buffer1(5);
             * buffer1.push(1);
             * buffer1.push(2);
             * 
             * CircularBuffer::CircularBufferDynamic<int> buffer2 = std::move(buffer1);
             * @endcode
             */
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
            /**
             * @brief Move assignment operator.
             *
             * Moves the values from another CircularBufferDynamic instance to this instance. This operator transfers ownership
             * of the other buffer's state to this buffer, leaving the other buffer in a valid but unspecified state.
             *
             * @param other The CircularBufferDynamic instance to move from.
             * @return A reference to this CircularBufferDynamic instance.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferDynamic<int> buffer1(5);
             * buffer1.push(1);
             * buffer1.push(2);
             * 
             * CircularBuffer::CircularBufferDynamic<int> buffer2(5);
             * buffer2 = std::move(buffer1);
             * @endcode
             */
            CircularBufferDynamic& operator=(CircularBufferDynamic&& other) noexcept {
                //--------------------------
                if(this == &other){
                    return *this;
                }//end if(this == &other)
                //--------------------------
                std::scoped_lock lock(m_mutex, other.m_mutex);
                //--------------------------
                m_max_size  = other.m_max_size;
                m_buffer    = std::move(other.m_buffer);
                //--------------------------
                if constexpr (std::is_arithmetic_v<T>) {
                    m_sum           = std::move(other.m_sum);
                    m_sum_squares   = std::move(other.m_sum_squares);
                }// end if constexpr (std::is_arithmetic_v<T>)
                //--------------------------
                return *this;
                //--------------------------
            }// end CircularBufferDynamic& operator=(const CircularBufferDynamic& other)
            //--------------------------
            CircularBufferDynamic(void)     = delete;
            //--------------------------
            ~CircularBufferDynamic(void)    = default;
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
             * CircularBuffer::CircularBufferDynamic<int> buffer(3);
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * buffer.push(4); // This will remove 1 from the front
             *
             * auto top = buffer.top();
             * if (top) {
             *     std::cout << "Top element: " << *top << std::endl; // Output: Top element: 2
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
             * CircularBuffer::CircularBufferDynamic<std::string> buffer(3);
             * std::string str = "Hello";
             * buffer.push(std::move(str));
             * buffer.push("World");
             *
             * auto top = buffer.top();
             * if (top) {
             *     std::cout << "Top element: " << *top << std::endl; // Output: Top element: Hello
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
             * CircularBuffer::CircularBufferDynamic<std::pair<int, std::string>> buffer(3);
             * buffer.emplace(1, "One");
             * buffer.emplace(2, "Two");
             * buffer.emplace(3, "Three");
             *
             * auto top = buffer.top();
             * if (top) {
             *     std::cout << "Top element: (" << top->first << ", " << top->second << ")" << std::endl; // Output: Top element: (1, One)
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
             * CircularBuffer::CircularBufferDynamic<int> buffer(3);
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
             * CircularBuffer::CircularBufferDynamic<int> buffer(3);
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * 
             * auto top = buffer.top();
             * if (top) {
             *     std::cout << "Top element: " << *top << std::endl; // Output: Top element: 1
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            std::optional<T> top(void) const{
                return get_top();
            }//end void top(void) const
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
             * CircularBuffer::CircularBufferDynamic<int> buffer(3);
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * 
             * auto last = buffer.last();
             * if (last) {
             *     std::cout << "Last element: " << *last << std::endl; // Output: Last element: 3
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
             * CircularBuffer::CircularBufferDynamic<int> buffer(3);
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * 
             * auto top = buffer.top_pop();
             * if (top) {
             *     std::cout << "Top element: " << *top << std::endl; // Output: Top element: 1
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * 
             * top = buffer.top_pop();
             * if (top) {
             *     std::cout << "Top element: " << *top << std::endl; // Output: Top element: 2
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
             * CircularBuffer::CircularBufferDynamic<int> buffer(3);
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
             * CircularBuffer::CircularBufferDynamic<int> buffer(3);
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
             * CircularBuffer::CircularBufferDynamic<int> buffer(3);
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
             * The sum is calculated using the following equation:
             * \f[
             * \text{sum} = \sum_{i=1}^{n} x_i
             * \f]
             * where \f$x_i\f$ is each element in the buffer.
             *
             * @return The sum of the elements, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferDynamic<int> buffer(5);
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
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                return get_sum();
                //--------------------------
            }// end std::optional<T> sum(void) const
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
             * CircularBuffer::CircularBufferDynamic<int> buffer(5);
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
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                return get_mean();
                //--------------------------
            }// end std::optional<T> mean(void) const
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
             * CircularBuffer::CircularBufferDynamic<int> buffer(5);
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * buffer.push(4);
             * buffer.push(5);
             * 
             * auto variance = buffer.variance();
             * if (variance) {
             *     std::cout << "Variance of elements: " << *variance << std::endl; // Output: Variance of elements: 2.5
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> variance(void) const {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                return get_variance();
                //--------------------------
            }// end std::optional<T> variance(void) const
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
             * CircularBuffer::CircularBufferDynamic<int> buffer(5);
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * buffer.push(4);
             * buffer.push(5);
             * 
             * auto stddev = buffer.standard_deviation();
             * if (stddev) {
             *     std::cout << "Standard deviation of elements: " << *stddev << std::endl; // Output: Standard deviation of elements: 1.58
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> standard_deviation(void) const {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                return get_standard_deviation();
                //--------------------------
            }// end std::optional<T> standard_deviation(void) const
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
             * CircularBuffer::CircularBufferDynamic<int> buffer(5);
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
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<U>> minimum(void) const {
                //--------------------------
                return get_min();
                //--------------------------
            }// end std::optional<T> min(void) const
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
             * CircularBuffer::CircularBufferDynamic<int> buffer(5);
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
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<U>> maximum(void) const {
                //--------------------------
                return get_max();
                //--------------------------
            }// end std::optional<T> max(void) const
            //--------------------------
            /**
             * @brief Returns a sorted copy of the buffer.
             *
             * This method returns a sorted copy of all elements currently stored in the buffer in ascending order. 
             * This function is only available for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * @return A sorted std::vector of the elements, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferDynamic<int> buffer(5);
             * buffer.push(3);
             * buffer.push(1);
             * buffer.push(4);
             * buffer.push(1);
             * buffer.push(5);
             * 
             * auto sorted = buffer.sorted();
             * if (sorted) {
             *     std::cout << "Sorted elements: ";
             *     for (const auto& elem : *sorted) {
             *         std::cout << elem << " "; // Output: Sorted elements: 1 1 3 4 5
             *     }
             *     std::cout << std::endl;
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<std::vector<U>>> sorted(void) const {
                //--------------------------
                return get_sorted();
                //--------------------------
            }// end std::optional<std::vector<T>> sorted(void) const
            //--------------------------
            /**
             * @brief Returns a reverse sorted copy of the buffer.
             *
             * This method returns a reverse sorted copy of all elements currently stored in the buffer in descending order.
             * This function is only available for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * @return A reverse sorted std::vector of the elements, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferDynamic<int> buffer(5);
             * buffer.push(3);
             * buffer.push(1);
             * buffer.push(4);
             * buffer.push(1);
             * buffer.push(5);
             * 
             * auto reverse_sorted = buffer.reverse_sorted();
             * if (reverse_sorted) {
             *     std::cout << "Reverse sorted elements: ";
             *     for (const auto& elem : *reverse_sorted) {
             *         std::cout << elem << " "; // Output: Reverse sorted elements: 5 4 3 1 1
             *     }
             *     std::cout << std::endl;
             * } else {
             *     std::cout << "Buffer is empty" << std::endl;
             * }
             * @endcode
             */
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<std::vector<U>>> reverse_sorted(void) const {
                //--------------------------
                return get_reverse_sorted();
                //--------------------------
            }// end std::optional<std::vector<T>> reverse_sorted(void) const
            //--------------------------
            /**
             * @brief Calculates the median of the elements in the buffer.
             *
             * This method returns the median of all elements currently stored in the buffer. This function is only available
             * for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * The median is obtained by first copying the elements to a temporary buffer, sorting the buffer, and then selecting
             * the middle value if the number of elements is odd, or the average of the two middle values if the number of elements
             * is even.
             *
             * The time complexity of obtaining the median is O(n log n) in the worst case due to the sorting step.
             *
             * @return The median of the elements, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBufferDynamic<int> buffer(5);
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
                    pop_front_unsafe();
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
                    pop_front_unsafe();
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
                    pop_front_unsafe();
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
            std::optional<T> get_top(void) const {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
                return m_buffer.front();
                //--------------------------
            }// end T get_top(void) const
            //--------------------------
            std::optional<T> get_last(void) const {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
                //--------------------------
                if (m_buffer.empty()) {
                    return std::nullopt;
                }// end if (m_buffer.empty())
                //--------------------------
                return m_buffer.back();
                //--------------------------
            }// end T get_last(void) const
            //--------------------------
            bool pop_front(void)  {
                //--------------------------
                std::unique_lock<std::mutex> lock(m_mutex);
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
                std::unique_lock<std::mutex> lock(m_mutex);
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
                std::unique_lock<std::mutex> lock(m_mutex);
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
                std::vector<T> _temp(half_size_ + 1);
                //--------------------------
                if (size_ % 2UL == 0UL) {
                    //--------------------------
                    // For even number of elements, need to find two middle elements
                    //--------------------------
#if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
                    std::partial_sort_copy(std::execution::par, m_buffer.begin(), m_buffer.end(), _temp.begin(), _temp.end());
#else
                    std::partial_sort_copy(m_buffer.begin(), m_buffer.end(), _temp.begin(), _temp.end());
#endif
                    //--------------------------
                    const double median_1 = static_cast<double>(_temp.at(half_size_));
                    const double median_2 = static_cast<double>(_temp.at(half_size_ - 1));
                    //--------------------------
                    return (median_1 + median_2) / 2.;
                    //--------------------------
                }// end if (size_ % 2UL == 0UL)
                //--------------------------
                // For odd number of elements, find the middle element
                //--------------------------
#if defined(HAS_TBB) && defined(BUILD_CIRCULARBUFFER_MULTI_THREADING)
                std::partial_sort_copy(std::execution::par, m_buffer.begin(), m_buffer.end(), _temp.begin(), _temp.end());
#else
                std::partial_sort_copy(m_buffer.begin(), m_buffer.end(), _temp.begin(), _temp.end());
#endif
                //--------------------------
                return static_cast<double>(_temp.at(half_size_));
                //--------------------------
            }// end std::optional<T> get_median(void) const
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            const size_t m_max_size;
            std::deque<T> m_buffer;
            typename std::conditional<std::is_arithmetic<T>::value, T, std::nullptr_t>::type m_sum, m_sum_squares;
            mutable std::mutex m_mutex;
            //--------------------------
            void pop_front_unsafe(void)  {
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
            }// end void pop_front_unsafe(void)
            //--------------------------------------------------------------
    };// end class CircularBufferDynamic
    //--------------------------------------------------------------
}// end namespace CircularBuffers
