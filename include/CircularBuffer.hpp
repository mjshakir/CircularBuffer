#pragma once
//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <optional>
#include <array>
#include <vector>
#include <span>
#include <atomic>
#include <type_traits>
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <functional>
#include <utility>
#include <iterator>
#if __has_include(<execution>)
    #if defined(HAS_TBB) && defined(CIRCULARBUFFER_MULTI_THREADING)
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
     * @class CircularBuffer
     * @brief A fixed-size or dynamic-size Lock-free circular buffer implementation.
     *
     * @details This class provides a fixed-size or dynamic-size  circular buffer for any data type T. It is thread-safe and 
     * includes additional statistical methods for arithmetic types. If the buffer is full, new elements will
     * overwrite the oldest elements. The buffer can be used with a fixed-size std::array or a dynamic-size std::vector.
     * It derives from its container through a protected base: a std::vector for N == 0, a std::array for N above 0, a
     * std::span for N == std::dynamic_extent (the objects belong to the caller; the buffer assigns into them and never
     * constructs or destroys them; the buffer cannot be copied, and a moved-from one has capacity 0). A user reaches
     * only the buffer's own functions and the iteration of the container (begin, end, rbegin, rend and the c-versions),
     * which walks the storage from slot 0, not the ring from the oldest item.
     *
     * @note The move constructor and the move assignment hand the storage over (a std::vector's block is stolen, a
     * std::array is moved element by element) and copy nothing else. They are valid only while no other thread uses
     * either buffer. The member functions try_emplace() and consume() are for ONE producer thread and ONE consumer
     * thread: try_emplace() never overwrites, and consume() reads the oldest item where it sits. Every other
     * function keeps its meaning, many producers and many consumers included.
     *
     * @tparam T Type of elements stored in the buffer.
     * @tparam N Size of the buffer.default value is 0
     *
     * @example usage:
     * @code
     * #include "CircularBuffer.hpp"
     *
     * int main() {
     *     CircularBuffer::CircularBuffer<int, 5> buffer;
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
    template <typename T, size_t N = 0UL>
    class CircularBuffer : protected std::conditional<(N == 0), std::vector<T>, typename std::conditional<(N == std::dynamic_extent), std::span<T>, std::array<T, N>>::type>::type {
        //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            using value_type             = T;
            using reference              = value_type&;
            using const_reference        = const value_type&;
            using pointer                = value_type*;
            using const_pointer          = const value_type*;
            using size_type              = size_t;
            using difference_type        = std::ptrdiff_t;
            using iterator               = typename std::conditional<(N == 0), std::vector<T>, typename std::conditional<(N == std::dynamic_extent), std::span<T>, std::array<T, N>>::type>::type::iterator;
            using const_iterator         = typename std::conditional<(N == std::dynamic_extent),
                                            typename std::span<const T>::iterator,
                                            typename std::conditional<(N == 0 || N == std::dynamic_extent), std::vector<T>, std::array<T, N>>::type::const_iterator>::type;
            using reverse_iterator       = typename std::conditional<(N == 0), std::vector<T>, typename std::conditional<(N == std::dynamic_extent), std::span<T>, std::array<T, N>>::type>::type::reverse_iterator;
            using const_reverse_iterator = typename std::conditional<(N == std::dynamic_extent),
                                            typename std::span<const T>::reverse_iterator,
                                            typename std::conditional<(N == 0 || N == std::dynamic_extent), std::vector<T>, std::array<T, N>>::type::const_reverse_iterator>::type;
            //--------------------------------------------------------------
        public:
            //--------------------------------------------------------------
            /**
             * @brief Default constructor for non-arithmetic types.
             *
             * @details Initializes the buffer with head, tail, and count set to 0.
             * This uses a std::array as the underlying buffer.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<std::string, 3> buffer;
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
            template <typename U = T, std::enable_if_t<!std::is_arithmetic<U>::value && ((N > 0 && N != std::dynamic_extent)), int> = 0>
            CircularBuffer(void) : m_head(0UL), m_tail(0UL), m_count(0UL) {
                //--------------------------
            }// end CircularBuffer(void)
            //--------------------------
            /**
             * @brief Default constructor for arithmetic types.
             *
             * @details Initializes the buffer with head, tail, count, sum, and sum of squares set to 0.
             * This uses a std::array as the underlying buffer.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
            template <typename U = T, std::enable_if_t<std::is_arithmetic<U>::value && ((N > 0 && N != std::dynamic_extent)), int> = 0>
            CircularBuffer(void) : m_head(0UL), m_tail(0UL), m_count(0UL), m_sum(0UL), m_sum_squares(0UL){
                //--------------------------
            }// end CircularBuffer(void)
            //--------------------------
            /**
             * @brief Default constructor for non-arithmetic types.
             * 
             * @details Initializes the buffer with head, tail, count, and max size set to 0.
             * This uses a std::vector as the underlying buffer.
             * 
             * @param size The maximum size of the buffer.
             * 
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<std::string> buffer(5);
             * 
             * buffer.push("Hello");
             * buffer.push("World");
             * 
             * auto top = buffer.top();
             * if (top) {
             *    std::cout << "Top element: " << *top << std::endl;
             * }
             * @endcode
             */
            template <typename U = T, std::enable_if_t<!std::is_arithmetic<U>::value && (N == 0), int> = 0>
            CircularBuffer(const size_type& size) : std::vector<T>(size), m_head(0UL), m_tail(0UL), m_count(0UL), m_max_size(size) {
                //--------------------------
            }// end CircularBuffer(void)
            //--------------------------
            /**
             * @brief Default constructor for arithmetic types.
             * 
             * @details Initializes the buffer with head, tail, count, sum, sum of squares, and max size set to 0.
             * This uses a std::vector as the underlying buffer.
             * 
             * @param size The maximum size of the buffer.
             * 
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int> buffer(5);
             * 
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * 
             * auto sum = buffer.sum();
             * if (sum) {
             *   std::cout << "Sum of elements: " << *sum << std::endl;
             * }
             * 
             * auto mean = buffer.mean();
             * if (mean) {
             *  std::cout << "Mean of elements: " << *mean << std::endl;
             * }
             * @endcode
             */
            template <typename U = T, std::enable_if_t<std::is_arithmetic<U>::value && (N == 0), int> = 0>
            CircularBuffer(const size_type& size) :    std::vector<T>(size),
                                                    m_head(0UL),
                                                    m_tail(0UL),
                                                    m_count(0UL),
                                                    m_sum(0UL),
                                                    m_sum_squares(0UL),
                                                    m_max_size(size) {
                //--------------------------
            }// end CircularBuffer(void)
            //--------------------------
            /**
             * @brief Constructor of a buffer whose slots are objects the caller owns, for non-arithmetic types.
             *
             * @details Initializes an empty buffer over storage: the capacity is storage.size(). The buffer allocates nothing
             * and neither constructs nor destroys the objects of storage; it assigns into them as it assigns into its own
             * slots. An empty storage gives a buffer of capacity 0, which stores nothing and answers every call. The
             * objects must outlive the buffer (a buffer moved to is the one that uses them) and be used by no one else
             * while the buffer lives. One thread constructs; afterwards the rules of the class for one producer thread and
             * one consumer thread hold unchanged. A copy of the buffer owns its own memory.
             *
             * @param storage The caller's objects; a temporary container is refused at compile time.
             *
             * @example usage:
             * @code
             * std::array<std::string, 3UL> memory;
             * CircularBuffer::CircularBuffer<std::string> buffer(memory);
             * std::cout << buffer.capacity() << std::endl; // Output: 3
             * @endcode
             */
            template <typename U = T, std::enable_if_t<!std::is_arithmetic<U>::value && (N == std::dynamic_extent), int> = 0>
            explicit CircularBuffer(std::span<T> storage) noexcept : std::span<T>(storage), m_head(0UL), m_tail(0UL), m_count(0UL), m_max_size(storage.size()) {
                //--------------------------
            }// end CircularBuffer(std::span<T> storage)
            //--------------------------
            /**
             * @brief Constructor of a buffer whose slots are objects the caller owns, for arithmetic types.
             *
             * @details As the constructor for non-arithmetic types, and head, tail, count, sum and sum of squares start
             * at 0.
             *
             * @param storage The caller's objects; a temporary container is refused at compile time.
             */
            template <typename U = T, std::enable_if_t<std::is_arithmetic<U>::value && (N == std::dynamic_extent), int> = 0>
            explicit CircularBuffer(std::span<T> storage) noexcept :
                                                    std::span<T>(storage),
                                                    m_head(0UL),
                                                    m_tail(0UL),
                                                    m_count(0UL),
                                                    m_sum(0UL),
                                                    m_sum_squares(0UL),
                                                    m_max_size(storage.size()) {
                //--------------------------
            }// end CircularBuffer(std::span<T> storage)
            //--------------------------
            /**
             * @brief Copy constructor.
             *
             * @details Initializes the buffer with the values from another CircularBuffer instance. This constructor
             * performs a deep copy of the other buffer's state, including the head, tail, count, and buffer contents.
             * If the data type T is arithmetic, the sum and sum of squares are also copied.
             *
             * @param other The CircularBuffer instance to copy from.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer1;
             * buffer1.push(1);
             * buffer1.push(2);
             * buffer1.push(3);
             *
             * CircularBuffer::CircularBuffer<int, 5> buffer2 = buffer1;
             *
             * auto top = buffer2.top();
             * if (top) {
             *     std::cout << "Top element in buffer2: " << *top << std::endl;
             * }
             * @endcode
             */
            CircularBuffer(const CircularBuffer& other) requires (N != std::dynamic_extent) :   std::conditional<(N == 0), std::vector<T>, typename std::conditional<(N == std::dynamic_extent), std::span<T>, std::array<T, N>>::type>::type(other),
                                                            m_head(other.m_head.load()), 
                                                            m_tail(other.m_tail.load()), 
                                                            m_count(other.m_count.load()){
                //--------------------------
                if constexpr (std::is_arithmetic_v<T>) {
                    m_sum           = other.m_sum.load();
                    m_sum_squares   = other.m_sum_squares.load();
                }//end if constexpr (std::is_arithmetic_v<T>)
                //--------------------------
                if constexpr ((N == 0 || N == std::dynamic_extent)) {
                    m_max_size  = other.m_max_size;
                }//end if constexpr ((N > 0 && N != std::dynamic_extent))
                //--------------------------
            }// end CircularBuffer(const CircularBuffer& other)
            //--------------------------
            // Two rings on one memory are refused; without this line the buffer would copy-construct as a view of the other's range.
            CircularBuffer(const CircularBuffer& other) requires (N == std::dynamic_extent) = delete;
            //--------------------------
            /**
             * @brief Copy assignment operator.
             *
             * @details Copies the values from another CircularBuffer instance. This operator performs a deep copy of the other
             * buffer's state, including the head, tail, count, and buffer contents. If the data type T is arithmetic, the sum
             * and sum of squares are also copied.
             *
             * @param other The CircularBuffer instance to copy from.
             * @return A reference to this CircularBuffer instance.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer1;
             * buffer1.push(1);
             * buffer1.push(2);
             * buffer1.push(3);
             *
             * CircularBuffer::CircularBuffer<int, 5> buffer2;
             * buffer2 = buffer1;
             *
             * auto top = buffer2.top();
             * if (top) {
             *     std::cout << "Top element in buffer2: " << *top << std::endl;
             * }
             * @endcode
             */
            CircularBuffer& operator=(const CircularBuffer& other) requires (N != std::dynamic_extent) {
                //--------------------------
                if(this == &other){
                    return *this;
                }//end if(this == &other)
                //--------------------------
                m_head              = other.m_head.load();
                m_tail              = other.m_tail.load();
                m_count             = other.m_count.load();
                std::conditional<(N == 0), std::vector<T>, typename std::conditional<(N == std::dynamic_extent), std::span<T>, std::array<T, N>>::type>::type::operator=(other);
                //--------------------------
                if constexpr (std::is_arithmetic_v<T>) {
                    m_sum           = other.m_sum.load();
                    m_sum_squares   = other.m_sum_squares.load();
                }//end if constexpr (std::is_arithmetic_v<T>)
                //--------------------------
                if constexpr ((N == 0 || N == std::dynamic_extent)) {
                    m_max_size      = other.m_max_size;
                }//end if constexpr ((N > 0 && N != std::dynamic_extent))
                //--------------------------
                return *this;
                //--------------------------
            }// end CircularBuffer& operator=(const CircularBuffer& other)
            //--------------------------
            CircularBuffer& operator=(const CircularBuffer& other) requires (N == std::dynamic_extent) = delete;
            //--------------------------
            /**
             * @brief Move constructor.
             *
             * @details Initializes the buffer by moving the values from another CircularBuffer instance. This constructor
             * transfers ownership of the other buffer's state to the new buffer. The storage is moved, not copied: the
             * block of a std::vector changes hands (its address stays), and a std::array is moved element by element.
             * The values of the atomics are carried over with explicit loads and stores. The other buffer is left empty
             * (count, head and tail 0; for a dynamic-size buffer also without storage and with size 0), so it can only be
             * destroyed or assigned to. Valid only while no other thread uses either buffer.
             *
             * @param other The CircularBuffer instance to move from.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer1;
             * buffer1.push(1);
             * buffer1.push(2);
             * buffer1.push(3);
             *
             * CircularBuffer::CircularBuffer<int, 5> buffer2 = std::move(buffer1);
             *
             * auto top = buffer2.top();
             * if (top) {
             *     std::cout << "Top element in buffer2: " << *top << std::endl;
             * }
             * @endcode
             */
            CircularBuffer(CircularBuffer&& other) noexcept((N == 0 || N == std::dynamic_extent) || std::is_nothrow_move_constructible_v<T>) :
                                                            std::conditional<(N == 0), std::vector<T>, typename std::conditional<(N == std::dynamic_extent), std::span<T>, std::array<T, N>>::type>::type(std::move(other)),
                                                            m_head(other.m_head.load(std::memory_order_relaxed)),
                                                            m_tail(other.m_tail.load(std::memory_order_relaxed)),
                                                            m_count(other.m_count.load(std::memory_order_relaxed)){
                //--------------------------
                if constexpr (std::is_arithmetic_v<T>) {
                    m_sum           = other.m_sum.load(std::memory_order_relaxed);
                    m_sum_squares   = other.m_sum_squares.load(std::memory_order_relaxed);
                    other.m_sum.store(T{}, std::memory_order_relaxed);
                    other.m_sum_squares.store(T{}, std::memory_order_relaxed);
                }//end if constexpr (std::is_arithmetic_v<T>)
                //--------------------------
                if constexpr ((N == 0 || N == std::dynamic_extent)) {
                    m_max_size          = other.m_max_size;
                    other.m_max_size    = 0UL;
                }//end if constexpr ((N == 0 || N == std::dynamic_extent))
                //--------------------------
                if constexpr (N == std::dynamic_extent) {
                    static_cast<std::span<T>&>(other) = std::span<T>(); // the source keeps no slots
                }//end if constexpr (N == std::dynamic_extent)
                //--------------------------
                other.m_head.store(0UL, std::memory_order_relaxed);
                other.m_tail.store(0UL, std::memory_order_relaxed);
                other.m_count.store(0UL, std::memory_order_relaxed);
                //--------------------------
            }// end CircularBuffer(CircularBuffer&& other)
            //--------------------------
            /**
             * @brief Move assignment operator.
             *
             * Moves the values from another CircularBuffer instance to this instance. This operator transfers ownership
             * of the other buffer's state to this buffer. The storage is moved, not copied, the values of the atomics are
             * carried over with explicit loads and stores, and the other buffer is left empty, as for the move
             * constructor. The elements this buffer held are given up. Valid only while no other thread uses either
             * buffer.
             *
             * @param other The CircularBuffer instance to move from.
             * @return A reference to this CircularBuffer instance.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer1;
             * buffer1.push(1);
             * buffer1.push(2);
             * buffer1.push(3);
             *
             * CircularBuffer::CircularBuffer<int, 5> buffer2;
             * buffer2 = std::move(buffer1);
             *
             * auto top = buffer2.top();
             * if (top) {
             *     std::cout << "Top element in buffer2: " << *top << std::endl;
             * }
             * @endcode
             */
            CircularBuffer& operator=(CircularBuffer&& other) noexcept((N == 0 || N == std::dynamic_extent) || std::is_nothrow_move_assignable_v<T>) {
                //--------------------------
                if(this == &other){
                    return *this;
                }//end if(this == &other)
                //--------------------------
                std::conditional<(N == 0), std::vector<T>, typename std::conditional<(N == std::dynamic_extent), std::span<T>, std::array<T, N>>::type>::type::operator=(std::move(other));
                m_head.store(other.m_head.load(std::memory_order_relaxed), std::memory_order_relaxed);
                m_tail.store(other.m_tail.load(std::memory_order_relaxed), std::memory_order_relaxed);
                m_count.store(other.m_count.load(std::memory_order_relaxed), std::memory_order_relaxed);
                //--------------------------
                if constexpr (std::is_arithmetic_v<T>) {
                    m_sum.store(other.m_sum.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    m_sum_squares.store(other.m_sum_squares.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    other.m_sum.store(T{}, std::memory_order_relaxed);
                    other.m_sum_squares.store(T{}, std::memory_order_relaxed);
                }//end if constexpr (std::is_arithmetic_v<T>)
                //--------------------------
                if constexpr ((N == 0 || N == std::dynamic_extent)) {
                    m_max_size          = other.m_max_size;
                    other.m_max_size    = 0UL;
                }//end if constexpr ((N == 0 || N == std::dynamic_extent))
                //--------------------------
                if constexpr (N == std::dynamic_extent) {
                    static_cast<std::span<T>&>(other) = std::span<T>(); // the source keeps no slots
                }//end if constexpr (N == std::dynamic_extent)
                //--------------------------
                other.m_head.store(0UL, std::memory_order_relaxed);
                other.m_tail.store(0UL, std::memory_order_relaxed);
                other.m_count.store(0UL, std::memory_order_relaxed);
                //--------------------------
                return *this;
                //--------------------------
            }// end CircularBuffer& operator=(CircularBuffer&& other)
            //--------------------------
            ~CircularBuffer(void) = default;
            //--------------------------
            /**
             * @brief Pushes an item to the back of the buffer.
             * A buffer with no slots (capacity 0) stores nothing: the call does nothing, as try_emplace() returns false.
             *
             * @details This method adds a new item to the back of the buffer. If the buffer is full, it will 
             * remove the item from the front to make space for the new item.
             *
             * @param item The item to push.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
             * A buffer with no slots (capacity 0) stores nothing: the call does nothing, as try_emplace() returns false.
             *
             * @details This method adds a new item to the back of the buffer, moving the item instead of copying it. 
             * If the buffer is full, it will remove the item from the front to make space for the new item.
             *
             * @param item The item to push.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<std::string, 5> buffer;
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
             * A buffer with no slots (capacity 0) stores nothing: the call does nothing, as try_emplace() returns false.
             *
             * @details This method constructs a new item in place at the back of the buffer. If the buffer is full, 
             * it will remove the item from the front to make space for the new item.
             *
             * @tparam Args Types of arguments for constructing the item.
             * @param args Arguments for constructing the item.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<std::pair<int, std::string>, 5> buffer;
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
             * @details This method removes the item at the front of the buffer. If the buffer is empty, it returns false.
             * If the buffer contains items, it removes the front item and returns true.
             *
             * @return True if an item was popped, false if the buffer was empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
             * @details This method returns the item at the front of the buffer without removing it. If the buffer is empty,
             * it returns std::nullopt.
             *
             * @return The top item, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
             * @details This method returns the item at the back of the buffer without removing it. If the buffer is empty,
             * it returns std::nullopt.
             *
             * @return The last item, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
             * @details This method returns the item at the front of the buffer and removes it. If the buffer is empty,
             * it returns std::nullopt.
             *
             * @return The top item, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
            std::optional<T> top_pop(void) {
                return get_top_pop();
            }// end std::optional<T> pop(void)
            //--------------------------
            /**
             * @brief Emplaces an item to the back of the buffer, or refuses when the buffer is full.
             *
             * @details This method constructs a new item in place at the back of the buffer, in its slot, with no copy
             * of the item. Unlike emplace() it never removes the oldest item: when the buffer is full it returns false
             * and changes nothing. For ONE producer thread (with, at most, one other thread consuming): only the
             * consumer lowers the count, so a buffer seen not full stays not full until this call has written its slot.
             *
             * @tparam Args Types of arguments for constructing the item.
             * @param args Arguments for constructing the item.
             * @return True if the item was constructed, false if the buffer was full.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<std::pair<int, std::string>, 2> buffer;
             * buffer.try_emplace(1, "One");   // true
             * buffer.try_emplace(2, "Two");   // true
             * buffer.try_emplace(3, "Three"); // false: the buffer is full and keeps 1 and 2
             * @endcode
             */
            template <typename... Args>
            bool try_emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>) {
                //--------------------------
                if (full()) {
                    return false;
                }// end if (full())
                //--------------------------
                emplace_back(std::forward<Args>(args)...);
                //--------------------------
                return true;
                //--------------------------
            }// end bool try_emplace(Args&&... args)
            //--------------------------
            /**
             * @brief Gives the oldest item to a function where it sits, then removes it.
             *
             * @details This method calls fn with a const reference to the oldest item of the buffer, then removes that
             * item. The item is not copied. If the buffer is empty it returns false and fn is not called. For ONE
             * consumer thread: a second consumer could remove the item while fn reads it. The slot goes back to the
             * producer only after fn has returned.
             *
             * @tparam Fn A callable taking a const reference to T.
             * @param fn The function that reads the oldest item.
             * @return True if an item was read and removed, false if the buffer was empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
             * buffer.push(1);
             * buffer.push(2);
             *
             * buffer.consume([](const int& item) { std::cout << "Oldest: " << item << std::endl; }); // Oldest: 1
             * @endcode
             */
            template <typename Fn>
            bool consume(Fn&& fn) noexcept(std::is_nothrow_invocable_v<Fn&&, const T&>) {
                //--------------------------
                if (is_empty()) {
                    return false;
                }// end if (is_empty())
                //--------------------------
                std::forward<Fn>(fn)(std::as_const(this->operator[](m_head.load(std::memory_order_acquire))));
                //--------------------------
                return pop_front();
                //--------------------------
            }// end bool consume(Fn&& fn)
            //--------------------------
            /**
             * @brief Checks if the buffer is empty.
             *
             * @details This method returns true if the buffer is empty (i.e., it contains no elements), and false otherwise.
             *
             * @return True if the buffer is empty, false otherwise.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
             * @details This method returns the current number of elements stored in the buffer.
             *
             * @return The number of items in the buffer.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
             * 
             * buffer.push(1);
             * buffer.push(2);
             * buffer.push(3);
             * 
             * std::cout << "Buffer size: " << buffer.size() << std::endl; // Output: Buffer size: 3
             * @endcode
             */
            size_type size(void) const {   
                return get_size();
            }// end size_type size(void) const
            //--------------------------
            /**
             * @brief Gets the maximum size of the buffer.
             *
             * @details This method returns the maximum number of elements that the buffer can store. If the buffer is dynamic-size,
             * it returns the current maximum size of the buffer.
             *
             * @return The maximum size of the buffer.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
             * 
             * std::cout << "Buffer max size: " << buffer.max_size() << std::endl; // Output: Buffer max size: 5
             * @endcode
             */
            size_type capacity(void) const {
                return get_capacity();
            }// end size_type capacity(void) const
            //--------------------------
            /**
             * @brief Clears the buffer.
             *
             * @details This method removes all elements from the buffer, effectively resetting it to its initial empty state.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
             * @details This method returns the sum of all elements currently stored in the buffer. This function is only available
             * for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * @return The sum of the elements, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
             * @details This method returns the mean (average) of all elements currently stored in the buffer. This function is only available
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
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
             * @details This method returns the variance of all elements currently stored in the buffer. This function is only available
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
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
             * @details This method returns the standard deviation of all elements currently stored in the buffer. This function is only available
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
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
             * @details This method returns the minimum element currently stored in the buffer. This function is only available
             * for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * @return The minimum element, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
             * @details This method returns the maximum element currently stored in the buffer. This function is only available
             * for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * @return The maximum element, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
             * @details This method returns a sorted copy of all elements currently stored in the buffer in ascending order. 
             * This function is only available for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * @return A sorted std::array of the elements, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
            std::enable_if_t<std::is_arithmetic<U>::value && ((N > 0 && N != std::dynamic_extent)), std::optional<std::array<T, N>>> sorted(void) const {
                return get_sorted();
            }//end std::optional<std::array<T, N>> sorted(void) const
            //--------------------------
            /**
             * @brief Returns a reverse sorted copy of the buffer.
             *
             * @details This method returns a reverse sorted copy of all elements currently stored in the buffer in descending order.
             * This function is only available for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * @return A reverse sorted std::array of the elements, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
            std::enable_if_t<std::is_arithmetic<U>::value && ((N > 0 && N != std::dynamic_extent)), std::optional<std::array<T, N>>> reverse_sorted(void) const {
                return get_reverse_sorted();
            }//end std::optional<std::array<T, N>> reverse_sorted(void) const
            //--------------------------
            /**
             * @brief Returns a sorted copy of the buffer.
             *
             * @details This method returns a sorted copy of all elements currently stored in the buffer in ascending order. 
             * This function is only available for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * @return A sorted std::array of the elements, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
            std::enable_if_t<std::is_arithmetic<U>::value && ((N == 0 || N == std::dynamic_extent)), std::optional<std::vector<T>>> sorted(void) const {
                return get_sorted();
            }//end std::optional<std::vector<T>> sorted(void) const
            //--------------------------
            /**
             * @brief Returns a reverse sorted copy of the buffer.
             *
             * @details This method returns a reverse sorted copy of all elements currently stored in the buffer in descending order.
             * This function is only available for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * @return A reverse sorted std::array of the elements, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
            std::enable_if_t<std::is_arithmetic<U>::value && ((N == 0 || N == std::dynamic_extent)), std::optional<std::vector<T>>> reverse_sorted(void) const {
                return get_reverse_sorted();
            }//end std::optional<std::vector<T>> reverse_sorted(void) const
            //--------------------------
            /**
             * @brief Calculates the median of the elements in the buffer.
             *
             * @details This method returns the median of all elements currently stored in the buffer. This function is only available
             * for arithmetic types. If the buffer is empty, it returns std::nullopt.
             *
             * The median is obtained by first copying the elements to a temporary buffer, using std::nth_element to partially sort
             * the buffer to find the median value. If the number of elements is odd, the middle value is returned. If the number of elements
             * is even, the average of the two middle values is returned.
             *
             * The time complexity of obtaining the median is O((N > 0 && N != std::dynamic_extent)) on average, but O(n^2) in the worst case due to the nth_element algorithm.
             *
             * @return The median of the elements, or std::nullopt if the buffer is empty.
             *
             * @example usage:
             * @code
             * CircularBuffer::CircularBuffer<int, 5> buffer;
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
            // What the container offers to walk its storage from slot 0 (not the ring from the oldest item); every base has these four.
            using std::conditional<(N == 0), std::vector<T>, typename std::conditional<(N == std::dynamic_extent), std::span<T>, std::array<T, N>>::type>::type::begin;
            using std::conditional<(N == 0), std::vector<T>, typename std::conditional<(N == std::dynamic_extent), std::span<T>, std::array<T, N>>::type>::type::end;
            using std::conditional<(N == 0), std::vector<T>, typename std::conditional<(N == std::dynamic_extent), std::span<T>, std::array<T, N>>::type>::type::rbegin;
            using std::conditional<(N == 0), std::vector<T>, typename std::conditional<(N == std::dynamic_extent), std::span<T>, std::array<T, N>>::type>::type::rend;
            //--------------------------
            /**
             * @brief Gets a read-only iterator to the first slot of the storage.
             *
             * @details The slot at index 0 of the storage, not the oldest item: items sit where the ring put them (top() gives the oldest). std::span has no cbegin() before C++23, so the borrowed form answers through a read-only span over the same memory.
             *
             * @return A read-only iterator to the first slot.
             */
            const_iterator cbegin(void) const noexcept {
                //--------------------------
                if constexpr (N == std::dynamic_extent) {
                    return std::span<const T>(static_cast<const std::span<T>&>(*this)).begin();
                } else {
                    return std::conditional<(N == 0), std::vector<T>, typename std::conditional<(N == std::dynamic_extent), std::span<T>, std::array<T, N>>::type>::type::cbegin();
                }// end if constexpr (N == std::dynamic_extent)
                //--------------------------
            }// end const_iterator cbegin(void) const
            //--------------------------
            /**
             * @brief Gets a read-only iterator past the last slot of the storage.
             *
             * @details The slot at index 0 of the storage, not the oldest item: items sit where the ring put them (top() gives the oldest). std::span has no cend() before C++23, so the borrowed form answers through a read-only span over the same memory.
             *
             * @return A read-only iterator one past the last slot.
             */
            const_iterator cend(void) const noexcept {
                //--------------------------
                if constexpr (N == std::dynamic_extent) {
                    return std::span<const T>(static_cast<const std::span<T>&>(*this)).end();
                } else {
                    return std::conditional<(N == 0), std::vector<T>, typename std::conditional<(N == std::dynamic_extent), std::span<T>, std::array<T, N>>::type>::type::cend();
                }// end if constexpr (N == std::dynamic_extent)
                //--------------------------
            }// end const_iterator cend(void) const
            //--------------------------
            /**
             * @brief Gets a read-only reverse iterator to the last slot of the storage.
             *
             * @details The slot at index 0 of the storage, not the oldest item: items sit where the ring put them (top() gives the oldest). std::span has no crbegin() before C++23, so the borrowed form answers through a read-only span over the same memory.
             *
             * @return A reverse iterator that starts at the last slot.
             */
            const_reverse_iterator crbegin(void) const noexcept {
                //--------------------------
                if constexpr (N == std::dynamic_extent) {
                    return std::span<const T>(static_cast<const std::span<T>&>(*this)).rbegin();
                } else {
                    return std::conditional<(N == 0), std::vector<T>, typename std::conditional<(N == std::dynamic_extent), std::span<T>, std::array<T, N>>::type>::type::crbegin();
                }// end if constexpr (N == std::dynamic_extent)
                //--------------------------
            }// end const_reverse_iterator crbegin(void) const
            //--------------------------
            /**
             * @brief Gets a read-only reverse iterator past the first slot of the storage.
             *
             * @details The slot at index 0 of the storage, not the oldest item: items sit where the ring put them (top() gives the oldest). std::span has no crend() before C++23, so the borrowed form answers through a read-only span over the same memory.
             *
             * @return A reverse iterator one past the first slot.
             */
            const_reverse_iterator crend(void) const noexcept {
                //--------------------------
                if constexpr (N == std::dynamic_extent) {
                    return std::span<const T>(static_cast<const std::span<T>&>(*this)).rend();
                } else {
                    return std::conditional<(N == 0), std::vector<T>, typename std::conditional<(N == std::dynamic_extent), std::span<T>, std::array<T, N>>::type>::type::crend();
                }// end if constexpr (N == std::dynamic_extent)
                //--------------------------
            }// end const_reverse_iterator crend(void) const
            //--------------------------------------------------------------
        protected:
            //--------------------------------------------------------------
            size_type insert(void) {
                //--------------------------
                size_type current_tail{0UL}, next_tail{0UL};
                //--------------------------
                do {
                    current_tail = m_tail.load(std::memory_order_relaxed);
                    //--------------------------
                    next_tail = increment(current_tail);
                    //--------------------------
                    if (full()) {
                        pop_front_unsafe();
                    }// end if (full())
                    //--------------------------
                } while (!m_tail.compare_exchange_weak(current_tail, next_tail, std::memory_order_release, std::memory_order_relaxed));
                //--------------------------
                return current_tail;
                //--------------------------
            } //end size_type insert(void)
            //--------------------------
            std::optional<size_type> read_insert(void) {
                //--------------------------
                size_type current_head{0}, next_head{0};
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
                return current_head;
                //--------------------------
            } //end std::optional<size_type> read_insert(void) 
            //--------------------------
            void push_back(const T& item) {
                //--------------------------
                if (get_capacity() == 0UL) {
                    return; // a buffer with no slots takes nothing
                }// end if (get_capacity() == 0UL)
                //--------------------------
                this->operator[](insert()) = item;
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value){
                    atomic_add(item);
                }//end if constexpr (std::is_arithmetic<T>::value)
                //--------------------------
                m_count.fetch_add(1, std::memory_order_release);
                //--------------------------
            }//end bool push_back(const T& item)
            //--------------------------
            void push_back(T&& item) {
                //--------------------------
                if (get_capacity() == 0UL) {
                    return; // a buffer with no slots takes nothing
                }// end if (get_capacity() == 0UL)
                //--------------------------
                const size_type current_tail = insert();
                //--------------------------
                this->operator[](current_tail) = std::move(item);
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value){
                    //--------------------------
                    atomic_add(this->operator[](current_tail));
                    //--------------------------
                }//end if constexpr (std::is_arithmetic<T>::value)
                //--------------------------
                m_count.fetch_add(1, std::memory_order_release);
                //--------------------------
            }//end bool push_back(const T& item)
            //--------------------------
            template <typename... Args>
            void emplace_back(Args&&... args) {
                //--------------------------
                if (get_capacity() == 0UL) {
                    return; // a buffer with no slots takes nothing
                }// end if (get_capacity() == 0UL)
                //--------------------------
                const size_type current_tail = insert();
                //--------------------------
                new (&this->operator[](current_tail)) T(std::forward<Args>(args)...);  // Construct in-place
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value){
                    //--------------------------
                    atomic_add(this->operator[](current_tail));
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
                return this->operator[](m_head.load(std::memory_order_acquire));
                //--------------------------
            }//end std::optional<T> get_top(void) const
            //--------------------------
            std::optional<T> get_last(void) const {
                //--------------------------
                if (is_empty()) {
                    return std::nullopt;
                }// end if (is_empty())
                //--------------------------
                size_type _tail_position = m_tail.load(std::memory_order_acquire);
                if constexpr((N > 0 && N != std::dynamic_extent)) {
                    _tail_position = (_tail_position == 0) ? (N - 1) : (_tail_position - 1);
                }// end if constexpr((N > 0 && N != std::dynamic_extent))
                //--------------------------
                if constexpr((N == 0 || N == std::dynamic_extent)) {
                    _tail_position = (_tail_position == 0) ? (m_max_size - 1) : (_tail_position - 1);
                }// end if constexpr((N == 0 || N == std::dynamic_extent))
                //--------------------------
                return this->operator[](_tail_position);
                //--------------------------
            }//end std::optional<T> get_last(void) const
            //--------------------------
            std::optional<T> get_top_pop(void)  {
                //--------------------------
                const std::optional<size_type> current_head = read_insert();
                //--------------------------
                if (!current_head) {
                    return std::nullopt;
                }// end if (!current_head)
                //--------------------------
                T value = this->operator[](current_head.value());
                //--------------------------
                this->operator[](current_head.value()).~T();
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
                const std::optional<size_type> current_head = read_insert();
                //--------------------------
                if (!current_head) {
                    return false;
                }// end if (!current_head)
                //--------------------------
                if constexpr (std::is_arithmetic<T>::value){
                    //--------------------------
                    atomic_sub(this->operator[](current_head.value()));
                    //--------------------------
                }//end if constexpr (std::is_arithmetic<T>::value)
                //--------------------------
                this->operator[](current_head.value()).~T();
                //--------------------------
                m_count.fetch_sub(1, std::memory_order_acq_rel);
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
            bool full(void) const {
                if constexpr ((N == 0 || N == std::dynamic_extent)) {
                    return m_count.load(std::memory_order_acquire) == m_max_size;
                } else {
                    return m_count.load(std::memory_order_acquire) == N;
                }
            }// end bool full(void) const            
            //--------------------------
            size_type get_size(void) const  {
                //--------------------------
                return m_count.load(std::memory_order_acquire);
                //--------------------------
            }// end size_type get_size(void) const
            //--------------------------
            size_type get_capacity(void) const {
                //--------------------------
                if constexpr ((N > 0 && N != std::dynamic_extent)) {
                    return N;
                } else {
                    return m_max_size;
                }// end if constexpr ((N > 0 && N != std::dynamic_extent))
                //--------------------------
            }// end size_type get_capacity(void) const
            //--------------------------
            typename std::conditional<((N == 0 || N == std::dynamic_extent)), std::vector<T>, std::array<T, N>>::type get_copy(void) const {
                //--------------------------
                if constexpr ((N == 0 || N == std::dynamic_extent)) {
                    return std::vector<T>(this->begin(), this->end());
                } else {
                    return *this;
                }// end if constexpr ((N == 0 || N == std::dynamic_extent))
                //--------------------------
            }// end typename std::conditional<((N == 0 || N == std::dynamic_extent)), std::vector<T>, std::array<T, N>>::type get_copy(void) const
            //--------------------------
            void clear(void) {
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
            size_type increment(const size_type& value) const {
                if constexpr((N == 0 || N == std::dynamic_extent)) {
                    return (value + 1) % (m_max_size == 0 ? 1 : m_max_size);
                } else {
                    return (value + 1) % N;
                }// end if constexpr((N == 0 || N == std::dynamic_extent))
                //--------------------------
            }// end size_type increment(const size_type& value) const
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
                const size_type count_ = m_count.load(std::memory_order_relaxed);
                //--------------------------
                if (count_ < 2) {
                    return std::nullopt;
                }//end if (count < 2)
                //--------------------------
                const double mean_ = get_mean().value_or(0.);
                //--------------------------
                return ((static_cast<double>(m_sum_squares.load(std::memory_order_relaxed)) / static_cast<double>(count_)) - std::pow<double>(mean_, 2)) * 
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
#if defined(HAS_TBB) && defined(CIRCULARBUFFER_MULTI_THREADING)
                return *std::min_element(std::execution::par, this->begin(), this->begin() + m_count.load(std::memory_order_relaxed));
#else
                return *std::min_element(this->begin(), this->begin() + m_count.load(std::memory_order_relaxed));
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
#if defined(HAS_TBB) && defined(CIRCULARBUFFER_MULTI_THREADING)
                return *std::max_element(std::execution::par, this->begin(), this->begin() + m_count.load(std::memory_order_relaxed));
#else
                return *std::max_element(this->begin(), this->begin() + m_count.load(std::memory_order_relaxed));
#endif
                //--------------------------
            }//end std::optional<T> get_max(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value && ((N > 0 && N != std::dynamic_extent)), std::optional<std::array<T, N>>> get_sorted(void) const {
                //--------------------------
                if(is_empty()){
                    return std::nullopt;
                }//end if(is_empty())
                //--------------------------
                std::array<T, N> sorted_buffer = *this;
#if defined(HAS_TBB) && defined(CIRCULARBUFFER_MULTI_THREADING)
                std::sort(std::execution::par, sorted_buffer.begin(), sorted_buffer.begin() + m_count.load(std::memory_order_relaxed));
#else
                std::sort(sorted_buffer.begin(), sorted_buffer.begin() + m_count.load(std::memory_order_relaxed));
#endif
                return sorted_buffer;
                //--------------------------
            }//end std::optional<std::array<T, N>> get_sorted(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value && ((N > 0 && N != std::dynamic_extent)), std::optional<std::array<T, N>>> get_reverse_sorted(void) const {
                //--------------------------
                if(is_empty()){
                    return std::nullopt;
                }//end if(is_empty())
                //--------------------------
                std::array<T, N> sorted_buffer = *this;
#if defined(HAS_TBB) && defined(CIRCULARBUFFER_MULTI_THREADING)
                std::sort(std::execution::par, sorted_buffer.begin(), sorted_buffer.begin() + m_count.load(std::memory_order_relaxed), std::greater<T>());
#else
                std::sort(sorted_buffer.begin(), sorted_buffer.begin() + m_count.load(std::memory_order_relaxed), std::greater<T>());
#endif
                return sorted_buffer;
                //--------------------------
            }//end std::optional<std::array<T, N>> get_reverse_sorted(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value && ((N == 0 || N == std::dynamic_extent)), std::optional<std::vector<T>>> get_sorted(void) const {
                //--------------------------
                if(is_empty()){
                    return std::nullopt;
                }//end if(is_empty())
                //--------------------------
                std::vector<T> sorted_buffer = get_copy();
#if defined(HAS_TBB) && defined(CIRCULARBUFFER_MULTI_THREADING)
                std::sort(std::execution::par, sorted_buffer.begin(), sorted_buffer.begin() + m_count.load(std::memory_order_relaxed));
#else
                std::sort(sorted_buffer.begin(), sorted_buffer.begin() + m_count.load(std::memory_order_relaxed));
#endif
                return sorted_buffer;
                //--------------------------
            }//end std::optional<std::vector<T>> get_sorted(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value && ((N == 0 || N == std::dynamic_extent)), std::optional<std::vector<T>>> get_reverse_sorted(void) const {
                //--------------------------
                if(is_empty()){
                    return std::nullopt;
                }//end if(is_empty())
                //--------------------------
                std::vector<T> sorted_buffer = get_copy();
#if defined(HAS_TBB) && defined(CIRCULARBUFFER_MULTI_THREADING)
                std::sort(std::execution::par, sorted_buffer.begin(), sorted_buffer.begin() + m_count.load(std::memory_order_relaxed), std::greater<T>());
#else
                std::sort(sorted_buffer.begin(), sorted_buffer.begin() + m_count.load(std::memory_order_relaxed), std::greater<T>());
#endif
                return sorted_buffer;
                //--------------------------
            }//end std::optional<std::vector<T>> get_reverse_sorted(void) const
            //--------------------------
            template <typename U = T>
            std::enable_if_t<std::is_arithmetic<U>::value, std::optional<double>> get_median(void) const {
                //--------------------------
                if(is_empty()){
                    return std::nullopt;
                }//end if(is_empty())
                //--------------------------
                typename std::conditional<((N == 0 || N == std::dynamic_extent)), std::vector<T>, std::array<T, N>>::type sorted_buffer = get_copy();
                const size_type count_         = m_count.load(std::memory_order_relaxed);
                const size_type half_count_    = count_ / 2UL;
                //--------------------------
                if (count_ == 1) {
                    return static_cast<double>(sorted_buffer.at(0));
                }//end if (count_ == 1)
                //--------------------------
#if defined(HAS_TBB) && defined(CIRCULARBUFFER_MULTI_THREADING)
                std::nth_element(std::execution::par, sorted_buffer.begin(), sorted_buffer.begin() + half_count_, sorted_buffer.begin() + count_);
#else
                std::nth_element(sorted_buffer.begin(), sorted_buffer.begin() + half_count_, sorted_buffer.begin() + count_);
#endif
                //--------------------------
                if (count_ % 2UL == 0) {
                    //--------------------------
                    const double median_1 = static_cast<double>(sorted_buffer.at(half_count_));
#if defined(HAS_TBB) && defined(CIRCULARBUFFER_MULTI_THREADING)
                    std::nth_element(std::execution::par, sorted_buffer.begin(), sorted_buffer.begin() + half_count_ - 1, sorted_buffer.begin() + count_);
#else
                    std::nth_element(sorted_buffer.begin(), sorted_buffer.begin() + half_count_ - 1, sorted_buffer.begin() + count_);
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
                    atomic_float_add(m_sum_squares, std::pow<T>(value, 2));
                } else {
                    m_sum.fetch_add(value, std::memory_order_relaxed);
                    m_sum_squares.fetch_add(std::pow<T>(value, 2), std::memory_order_relaxed);
                }//end if constexpr (std::is_floating_point<T>::value)
                //--------------------------
            }//end void atomic_add(std::atomic<T>& atomic, const T& value)
            //--------------------------
            template <typename U = T, typename R>
            std::enable_if_t<std::is_arithmetic<U>::value, void> atomic_sub(const R& value) {
                //--------------------------
                if constexpr (std::is_floating_point<T>::value){
                    atomic_float_sub(m_sum, value);
                    atomic_float_sub(m_sum_squares, std::pow<T>(value, 2));
                } else {
                    m_sum.fetch_sub(value, std::memory_order_relaxed);
                    m_sum_squares.fetch_sub(std::pow<T>(value, 2), std::memory_order_relaxed);
                }//end if constexpr (std::is_floating_point<T>::value)
                //--------------------------
            }//end void atomic_sub(std::atomic<T>& atomic, const T& value)
            //--------------------------------------------------------------
        private:
            //--------------------------------------------------------------
            std::atomic<size_type> m_head, m_tail, m_count;
            typename std::conditional<std::is_arithmetic<T>::value, std::atomic<T>, std::nullptr_t>::type m_sum, m_sum_squares;
            typename std::conditional<((N == 0 || N == std::dynamic_extent)), size_type, std::nullptr_t>::type m_max_size;
            //--------------------------
            void pop_front_unsafe(void)  {
                //--------------------------
                size_type current_head{0}, next_head{0};
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
                    atomic_sub(this->operator[](current_head));
                    //--------------------------
                }//end if constexpr (std::is_arithmetic<T>::value)
                //--------------------------
                this->operator[](current_head).~T();
                //--------------------------
                // Decrement the count of elements
                //--------------------------
                m_count.fetch_sub(1, std::memory_order_acq_rel); // Using acq_rel for decrement to ensure total order with increment operations
                //--------------------------
            }// end void pop_front_unsafe(void)
        //--------------------------------------------------------------
    };//end class CircularBuffer
    //--------------------------------------------------------------
}// end namespace CircularBuffer