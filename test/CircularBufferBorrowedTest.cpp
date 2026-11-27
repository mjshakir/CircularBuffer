//--------------------------------------------------------------
// Standard cpp library
//--------------------------------------------------------------
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <optional>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
//--------------------------------------------------------------
// GTest library
//--------------------------------------------------------------
#include <gtest/gtest.h>
//--------------------------------------------------------------
// User Defined library
//--------------------------------------------------------------
#include "CircularBuffer.hpp"
//--------------------------------------------------------------
// Counts every call of the global allocation functions, so a test can show that a buffer on the caller's memory never allocates.
//--------------------------------------------------------------
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
    #define CIRCULARBUFFER_TEST_SANITIZED 1
#elif defined(__has_feature)
    #if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
        #define CIRCULARBUFFER_TEST_SANITIZED 1
    #endif
#endif
namespace {
    std::atomic<std::size_t> g_new_calls{0UL};
    std::atomic<std::size_t> g_delete_calls{0UL};
}// end namespace

#ifndef CIRCULARBUFFER_TEST_SANITIZED
void* operator new(std::size_t size) {
    g_new_calls.fetch_add(1UL, std::memory_order_relaxed);
    void* pointer = std::malloc(size == 0UL ? 1UL : size);
    if (pointer == nullptr) {
        throw std::bad_alloc();
    }// end if (pointer == nullptr)
    return pointer;
}// end void* operator new(std::size_t size)

void operator delete(void* pointer) noexcept {
    g_delete_calls.fetch_add(1UL, std::memory_order_relaxed);
    std::free(pointer);
}// end void operator delete(void* pointer)

void operator delete(void* pointer, std::size_t) noexcept {
    g_delete_calls.fetch_add(1UL, std::memory_order_relaxed);
    std::free(pointer);
}// end void operator delete(void* pointer, std::size_t)
#endif
//--------------------------------------------------------------
struct Item40 {
    std::uint64_t v[5];
    Item40(const std::uint64_t& x = 0UL) : v{x, x + 1UL, x + 2UL, x + 3UL, x + 4UL} {}
    std::uint64_t key(void) const { return v[0]; }
    bool operator==(const Item40& other) const { return v[0] == other.v[0] && v[4] == other.v[4]; }
};
struct Item64 {
    std::uint64_t v[8];
    Item64(const std::uint64_t& x = 0UL) : v{x, x + 1UL, x + 2UL, x + 3UL, x + 4UL, x + 5UL, x + 6UL, x + 7UL} {}
    std::uint64_t key(void) const { return v[0]; }
    bool operator==(const Item64& other) const { return v[0] == other.v[0] && v[7] == other.v[7]; }
};
struct alignas(64) ItemLine {
    std::uint64_t v[8];
    ItemLine(const std::uint64_t& x = 0UL) : v{x, x + 1UL, x + 2UL, x + 3UL, x + 4UL, x + 5UL, x + 6UL, x + 7UL} {}
    std::uint64_t key(void) const { return v[0]; }
    bool operator==(const ItemLine& other) const { return v[0] == other.v[0] && v[7] == other.v[7]; }
};
struct Counted {
    static inline std::atomic<std::size_t> constructed{0UL};
    static inline std::atomic<std::size_t> destroyed{0UL};
    std::uint64_t value;
    Counted(void) : value(0UL) { constructed.fetch_add(1UL); }
    Counted(const std::uint64_t& x) : value(x) { constructed.fetch_add(1UL); }
    Counted(const Counted& other) : value(other.value) { constructed.fetch_add(1UL); }
    Counted& operator=(const Counted&) = default;
    ~Counted(void) { destroyed.fetch_add(1UL); }
    std::uint64_t key(void) const { return value; }
    bool operator==(const Counted& other) const { return value == other.value; }
    static void reset(void) { constructed = 0UL; destroyed = 0UL; }
};
template <typename T>
std::uint64_t key_of(const T& item) {
    if constexpr (std::is_arithmetic_v<T>) {
        return static_cast<std::uint64_t>(item);
    } else {
        return item.key();
    }// end if constexpr (std::is_arithmetic_v<T>)
}// end std::uint64_t key_of(const T& item)

// Objects the caller owns: CAPACITY slots for the buffer and a few more after them, which the buffer must not touch.
template <typename T, std::size_t CAPACITY>
struct Memory {
    static constexpr std::size_t EXTRA = 4UL;
    std::array<T, CAPACITY + EXTRA> items{};
    std::span<T> exact(void) { return std::span<T>(items.data(), CAPACITY); }
    std::span<T> larger(void) { return std::span<T>(items.data(), CAPACITY + EXTRA); }
    bool untouched(const std::size_t& from) const {
        for (std::size_t i = from; i < items.size(); ++i) {
            if (!(items[i] == T())) {
                return false;
            }// end if (!(items[i] == T()))
        }// end for (i)
        return true;
    }// end bool untouched(const std::size_t& from) const
};

template <typename T>
void check_exact_memory(void) {
    constexpr std::size_t CAPACITY = 7UL;
    Memory<T, CAPACITY> memory;
    CircularBuffer::CircularBuffer<T, std::dynamic_extent> buffer(memory.exact());
    ASSERT_EQ(CAPACITY, buffer.capacity());
    EXPECT_TRUE(buffer.empty());
    std::uint64_t next = 0UL, expected = 0UL;
    for (std::size_t lap = 0UL; lap < 3UL; ++lap) {
        for (std::size_t i = 0UL; i < CAPACITY; ++i) {
            ASSERT_TRUE(buffer.try_emplace(T(static_cast<std::uint64_t>(next++))));
        }// end for (i)
        EXPECT_EQ(CAPACITY, buffer.size());
        EXPECT_FALSE(buffer.try_emplace(T(static_cast<std::uint64_t>(9999UL))));
        for (std::size_t i = 0UL; i < CAPACITY; ++i) {
            std::uint64_t seen = 0UL;
            ASSERT_TRUE(buffer.consume([&seen](const T& item) { seen = key_of(item); }));
            EXPECT_EQ(expected++, seen);
        }// end for (i)
        EXPECT_TRUE(buffer.empty());
    }// end for (lap)
    for (std::size_t i = 0UL; i < CAPACITY + 2UL; ++i) {
        buffer.push(T(static_cast<std::uint64_t>(100UL + i)));
    }// end for (i)
    EXPECT_EQ(CAPACITY, buffer.size());
    EXPECT_EQ(102UL, key_of(buffer.top().value()));
    EXPECT_TRUE(memory.untouched(CAPACITY));
}// end void check_exact_memory(void)

TEST(CircularBufferCallerMemoryTest, ExactMemoryInt)    { check_exact_memory<int>(); }
TEST(CircularBufferCallerMemoryTest, ExactMemory40)     { check_exact_memory<Item40>(); }
TEST(CircularBufferCallerMemoryTest, ExactMemory64)     { check_exact_memory<Item64>(); }
TEST(CircularBufferCallerMemoryTest, ExactMemoryLine)   { check_exact_memory<ItemLine>(); }

TEST(CircularBufferCallerMemoryTest, ASpanOverAnArrayAVectorAndASubspanIsAcceptedByTheFullType) {
    std::array<int, 5UL> array{};
    std::vector<int> vector(6UL);
    const std::span<int> over_array(array);
    const std::span<int> over_vector(vector);
    const std::span<int> over_subspan = over_vector.subspan(1UL, 3UL);
    CircularBuffer::CircularBuffer<int, std::dynamic_extent> from_array(over_array);
    CircularBuffer::CircularBuffer<int, std::dynamic_extent> from_vector(over_vector);
    CircularBuffer::CircularBuffer<int, std::dynamic_extent> from_subspan(over_subspan);
    static_assert(!std::is_constructible_v<CircularBuffer::CircularBuffer<int>, std::span<int>>);
    static_assert(!std::is_constructible_v<CircularBuffer::CircularBuffer<int, std::dynamic_extent>, std::size_t>);
    EXPECT_EQ(5UL, from_array.capacity());
    EXPECT_EQ(6UL, from_vector.capacity());
    EXPECT_EQ(3UL, from_subspan.capacity());
    EXPECT_TRUE(from_array.try_emplace(7));
    EXPECT_EQ(7, array[0]);
}

template <typename B> constexpr bool names_iterators = requires {
    typename B::iterator; typename B::const_iterator; typename B::reverse_iterator; typename B::const_reverse_iterator; };
static_assert(names_iterators<CircularBuffer::CircularBuffer<int>>);
static_assert(names_iterators<CircularBuffer::CircularBuffer<int, 4UL>>);
static_assert(names_iterators<CircularBuffer::CircularBuffer<int, std::dynamic_extent>>);

TEST(CircularBufferCallerMemoryTest, TheConstIteratorsOfTheStorageAreNamedAndCallable) {
    std::array<int, 4UL> array{{9, 8, 7, 6}};
    const std::span<int> memory(array);
    const CircularBuffer::CircularBuffer<int, std::dynamic_extent> borrowed(memory);
    const CircularBuffer::CircularBuffer<int> owning_vector(4UL);
    const CircularBuffer::CircularBuffer<int, 4UL> owning_array;
    CircularBuffer::CircularBuffer<int, std::dynamic_extent>::const_iterator b_first = borrowed.cbegin();
    CircularBuffer::CircularBuffer<int, std::dynamic_extent>::const_reverse_iterator b_last = borrowed.crbegin();
    CircularBuffer::CircularBuffer<int>::const_iterator v_first = owning_vector.cbegin();
    CircularBuffer::CircularBuffer<int>::const_reverse_iterator v_last = owning_vector.crbegin();
    CircularBuffer::CircularBuffer<int, 4UL>::const_iterator a_first = owning_array.cbegin();
    CircularBuffer::CircularBuffer<int, 4UL>::const_reverse_iterator a_last = owning_array.crbegin();
    EXPECT_EQ(4L, borrowed.cend() - b_first);
    EXPECT_EQ(4L, borrowed.crend() - b_last);
    EXPECT_EQ(static_cast<std::ptrdiff_t>(owning_vector.capacity()), owning_vector.cend() - v_first);
    EXPECT_EQ(static_cast<std::ptrdiff_t>(owning_vector.capacity()), owning_vector.crend() - v_last);
    EXPECT_EQ(4L, owning_array.cend() - a_first);
    EXPECT_EQ(4L, owning_array.crend() - a_last);
    static_assert(std::is_same_v<decltype(*borrowed.cbegin()), const int&>);
    EXPECT_EQ(&*b_first, array.data());
    EXPECT_EQ(9, *b_first);
}

TEST(CircularBufferCallerMemoryTest, AnEmptySpanIsACapacityZeroBufferThatAnswersEverything) {
    CircularBuffer::CircularBuffer<Item40, std::dynamic_extent> buffer(std::span<Item40>{});
    EXPECT_EQ(0UL, buffer.capacity());
    EXPECT_EQ(0UL, buffer.size());
    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.try_emplace(1UL));
    buffer.push(Item40(1UL));
    buffer.emplace(2UL);
    EXPECT_FALSE(buffer.consume([](const Item40&) {}));
    EXPECT_FALSE(buffer.pop());
    EXPECT_FALSE(buffer.top().has_value());
    EXPECT_FALSE(buffer.last().has_value());
    EXPECT_FALSE(buffer.top_pop().has_value());
    EXPECT_EQ(0UL, buffer.size());
    buffer.reset();
    EXPECT_EQ(buffer.cbegin(), buffer.cend());
}

TEST(CircularBufferCallerMemoryTest, ElementsOutsideTheSpanAreUntouched) {
    constexpr std::size_t CAPACITY = 5UL;
    Memory<Item64, CAPACITY> memory;
    {
        CircularBuffer::CircularBuffer<Item64, std::dynamic_extent> buffer(memory.exact());
        for (std::size_t i = 0UL; i < 3UL * CAPACITY; ++i) {
            buffer.push(Item64(i));
        }// end for (i)
    }
    EXPECT_TRUE(memory.untouched(CAPACITY));
}

TEST(CircularBufferCallerMemoryTest, TheBufferNeitherConstructsNorDestroysTheCallersObjects) {
    constexpr std::size_t CAPACITY = 6UL;
    Counted::reset();
    {
        Memory<Counted, CAPACITY> memory;
        const std::size_t constructed = Counted::constructed.load();
        EXPECT_EQ((CAPACITY + Memory<Counted, CAPACITY>::EXTRA), constructed);
        {
            CircularBuffer::CircularBuffer<Counted, std::dynamic_extent> buffer(memory.exact());
            CircularBuffer::CircularBuffer<Counted, std::dynamic_extent> moved(std::move(buffer));
            EXPECT_EQ(CAPACITY, moved.capacity());
            EXPECT_EQ(constructed, Counted::constructed.load());
            EXPECT_EQ(0UL, Counted::destroyed.load());
        }
        EXPECT_EQ(constructed, Counted::constructed.load());
        EXPECT_EQ(0UL, Counted::destroyed.load());
    }
    EXPECT_EQ((CAPACITY + Memory<Counted, CAPACITY>::EXTRA), Counted::destroyed.load());
}

TEST(CircularBufferCallerMemoryTest, NothingIsAllocatedOrFreedInTheWholeLife) {
#ifdef CIRCULARBUFFER_TEST_SANITIZED
    GTEST_SKIP() << "counting needs the global allocation functions; the sanitizer checks every free instead";
#endif
    constexpr std::size_t CAPACITY = 8UL;
    Memory<Item40, CAPACITY> memory;
    Memory<Item40, CAPACITY> other;
    const std::size_t new_before    = g_new_calls.load();
    const std::size_t delete_before = g_delete_calls.load();
    {
        CircularBuffer::CircularBuffer<Item40, std::dynamic_extent> buffer(memory.exact());
        for (std::size_t i = 0UL; i < 3UL * CAPACITY; ++i) {
            buffer.push(Item40(i));
            buffer.try_emplace(i);
            buffer.pop();
        }// end for (i)
        CircularBuffer::CircularBuffer<Item40, std::dynamic_extent> moved(std::move(buffer));
        CircularBuffer::CircularBuffer<Item40, std::dynamic_extent> assigned(other.exact());
        assigned = std::move(moved);
        CircularBuffer::CircularBuffer<Item40, std::dynamic_extent> empty(std::span<Item40>{});
        empty = std::move(assigned);
        EXPECT_EQ(CAPACITY, empty.capacity());
        EXPECT_EQ(0UL, buffer.capacity());
        buffer.push(Item40(1UL));
        buffer.reset();
    }
    EXPECT_EQ(new_before, g_new_calls.load());
    EXPECT_EQ(delete_before, g_delete_calls.load());
}

TEST(CircularBufferCallerMemoryTest, MoveConstructionHandsTheSlotsOver) {
    constexpr std::size_t CAPACITY = 4UL;
    Memory<Item40, CAPACITY> memory;
    CircularBuffer::CircularBuffer<Item40, std::dynamic_extent> source(memory.exact());
    ASSERT_TRUE(source.try_emplace(11UL));
    ASSERT_TRUE(source.try_emplace(12UL));
    CircularBuffer::CircularBuffer<Item40, std::dynamic_extent> moved(std::move(source));
    EXPECT_EQ(0UL, source.capacity());
    EXPECT_EQ(0UL, source.size());
    EXPECT_FALSE(source.try_emplace(1UL));
    EXPECT_EQ(CAPACITY, moved.capacity());
    EXPECT_EQ(2UL, moved.size());
    EXPECT_EQ(11UL, moved.top().value().key());
    EXPECT_EQ(11UL, memory.items[0].key());
    EXPECT_EQ(&*moved.cbegin(), memory.items.data());
}

TEST(CircularBufferCallerMemoryTest, MoveAssignmentBetweenBuffersOnCallerMemory) {
    Memory<int, 3UL> memory_a, memory_b;
    CircularBuffer::CircularBuffer<int, std::dynamic_extent> destination(memory_a.exact());
    CircularBuffer::CircularBuffer<int, std::dynamic_extent> origin(memory_b.exact());
    ASSERT_TRUE(origin.try_emplace(5));
    destination = std::move(origin);
    EXPECT_EQ(3UL, destination.capacity());
    EXPECT_EQ(0UL, origin.capacity());
    EXPECT_EQ(0UL, origin.size());
    EXPECT_EQ(5, destination.top().value());
    EXPECT_EQ(5, memory_b.items[0]);
    EXPECT_EQ(&*destination.cbegin(), memory_b.items.data());
    EXPECT_TRUE(memory_a.untouched(0UL));
}

template <typename B> constexpr bool has_at             = requires(B& b) { b.at(0UL); };
template <typename B> constexpr bool has_data           = requires(B& b) { b.data(); };
template <typename B> constexpr bool has_index          = requires(B& b) { b[0UL]; };
template <typename B> constexpr bool has_front          = requires(B& b) { b.front(); };
template <typename B> constexpr bool has_back           = requires(B& b) { b.back(); };
template <typename B> constexpr bool has_resize         = requires(B& b) { b.resize(1UL); };
template <typename B> constexpr bool has_reserve        = requires(B& b) { b.reserve(1UL); };
template <typename B> constexpr bool has_shrink         = requires(B& b) { b.shrink_to_fit(); };
template <typename B> constexpr bool has_erase          = requires(B& b) { b.erase(b.begin()); };
template <typename B> constexpr bool has_assign         = requires(B& b) { b.assign(1UL, 1); };
template <typename B> constexpr bool has_pop_back       = requires(B& b) { b.pop_back(); };
template <typename B> constexpr bool has_push_back      = requires(B& b, const int& i) { b.push_back(i); };
template <typename B> constexpr bool has_emplace_back   = requires(B& b, const int& i) { b.emplace_back(i); };
template <typename B> constexpr bool has_insert         = requires(B& b, const int& i) { b.insert(b.begin(), i); };
template <typename B> constexpr bool has_swap_member    = requires(B& b, B& c) { b.swap(c); };
template <typename B> constexpr bool has_fill           = requires(B& b) { b.fill(1); };
template <typename B> constexpr bool has_max_size       = requires(B& b) { b.max_size(); };
template <typename B> constexpr bool has_get_allocator  = requires(B& b) { b.get_allocator(); };
template <typename B> constexpr bool has_first          = requires(B& b) { b.first(1UL); };
template <typename B> constexpr bool has_subspan        = requires(B& b) { b.subspan(1UL); };
template <typename B> constexpr bool has_size_bytes     = requires(B& b) { b.size_bytes(); };
template <typename B> constexpr bool has_status         = requires(B& b) { b.status(); };
template <typename B> constexpr bool has_ready          = requires(B& b) { b.ready(); };
template <typename B> constexpr bool has_iteration      = requires(B& b, const B& c) { b.begin(); b.end(); b.rbegin(); b.rend(); c.cbegin(); c.cend(); c.crbegin(); c.crend(); };
template <typename B> constexpr bool has_sorted         = requires(B& b) { b.sorted(); };
template <typename B> constexpr bool has_median         = requires(B& b) { b.median(); };

template <typename B>
constexpr bool container_is_unreachable(void) {
    return !has_at<B> && !has_data<B> && !has_index<B> && !has_front<B> && !has_back<B> && !has_resize<B> && !has_reserve<B> && !has_shrink<B> &&
           !has_erase<B> && !has_assign<B> && !has_pop_back<B> && !has_push_back<B> && !has_emplace_back<B> && !has_insert<B> && !has_swap_member<B> &&
           !has_fill<B> && !has_max_size<B> && !has_get_allocator<B> && !has_first<B> && !has_subspan<B> && !has_size_bytes<B> && !has_status<B> && !has_ready<B>;
}// end constexpr bool container_is_unreachable(void)

TEST(CircularBufferCallerMemoryTest, ThreeFormsAndTheContainerIsUnreachable) {
    static_assert(std::is_constructible_v<CircularBuffer::CircularBuffer<int>, std::size_t>);
    static_assert(std::is_constructible_v<CircularBuffer::CircularBuffer<int, std::dynamic_extent>, std::span<int>>);
    static_assert(std::is_constructible_v<CircularBuffer::CircularBuffer<int, std::dynamic_extent>, std::array<int, 4UL>&>);
    static_assert(std::is_constructible_v<CircularBuffer::CircularBuffer<int, std::dynamic_extent>, std::vector<int>&>);
    static_assert(!std::is_constructible_v<CircularBuffer::CircularBuffer<int, std::dynamic_extent>, std::vector<int>&&>);
    static_assert(!std::is_constructible_v<CircularBuffer::CircularBuffer<int, std::dynamic_extent>, std::array<int, 4UL>&&>);
    static_assert(!std::is_constructible_v<CircularBuffer::CircularBuffer<int, std::dynamic_extent>, const std::array<int, 4UL>&>);
    static_assert(!std::is_convertible_v<std::span<int>, CircularBuffer::CircularBuffer<int, std::dynamic_extent>>);
    static_assert(!std::is_constructible_v<CircularBuffer::CircularBuffer<int>, std::span<int>>);
    static_assert(!std::is_constructible_v<CircularBuffer::CircularBuffer<int, 4UL>, std::span<int>>);
    static_assert(!std::is_constructible_v<CircularBuffer::CircularBuffer<int, std::dynamic_extent>, std::size_t>);
    // copy: two rings on one memory are refused; the owning forms copy as before
    static_assert(!std::is_copy_constructible_v<CircularBuffer::CircularBuffer<int, std::dynamic_extent>>);
    static_assert(!std::is_copy_assignable_v<CircularBuffer::CircularBuffer<int, std::dynamic_extent>>);
    static_assert(std::is_move_constructible_v<CircularBuffer::CircularBuffer<int, std::dynamic_extent>> && std::is_move_assignable_v<CircularBuffer::CircularBuffer<int, std::dynamic_extent>>);
    static_assert(std::is_copy_constructible_v<CircularBuffer::CircularBuffer<int>> && std::is_copy_assignable_v<CircularBuffer::CircularBuffer<int>>);
    static_assert(std::is_copy_constructible_v<CircularBuffer::CircularBuffer<int, 4UL>> && std::is_copy_assignable_v<CircularBuffer::CircularBuffer<int, 4UL>>);
    // no conversion between the forms
    static_assert(!std::is_constructible_v<CircularBuffer::CircularBuffer<int>, CircularBuffer::CircularBuffer<int, std::dynamic_extent>&&>);
    static_assert(!std::is_constructible_v<CircularBuffer::CircularBuffer<int, std::dynamic_extent>, CircularBuffer::CircularBuffer<int>&&>);
    static_assert(!std::is_assignable_v<CircularBuffer::CircularBuffer<int>&, CircularBuffer::CircularBuffer<int, std::dynamic_extent>&&>);
    // the iteration the library always offered stays; nothing else of the container is reachable, in each form
    static_assert(has_iteration<CircularBuffer::CircularBuffer<int>> && has_iteration<CircularBuffer::CircularBuffer<int, 4UL>> && has_iteration<CircularBuffer::CircularBuffer<int, std::dynamic_extent>>);
    static_assert(container_is_unreachable<CircularBuffer::CircularBuffer<int>>());
    static_assert(container_is_unreachable<CircularBuffer::CircularBuffer<Item40>>());
    static_assert(container_is_unreachable<CircularBuffer::CircularBuffer<std::string>>());
    static_assert(container_is_unreachable<CircularBuffer::CircularBuffer<int, 4UL>>());
    static_assert(container_is_unreachable<CircularBuffer::CircularBuffer<int, std::dynamic_extent>>());
    static_assert(container_is_unreachable<CircularBuffer::CircularBuffer<std::string, std::dynamic_extent>>());
    static_assert(has_sorted<CircularBuffer::CircularBuffer<int>> && has_median<CircularBuffer::CircularBuffer<int, std::dynamic_extent>> && has_sorted<CircularBuffer::CircularBuffer<int, 4UL>>);
    static_assert(!std::is_convertible_v<CircularBuffer::CircularBuffer<int>&, std::vector<int>&>);
    static_assert(!std::is_convertible_v<CircularBuffer::CircularBuffer<int, 4UL>&, std::array<int, 4UL>&>);
    static_assert(!std::is_convertible_v<CircularBuffer::CircularBuffer<int, std::dynamic_extent>&, std::span<int>&>);
    SUCCEED();
}

TEST(CircularBufferCallerMemoryTest, CapacityZeroPushAndEmplaceStoreNothing) {
    CircularBuffer::CircularBuffer<int> owned(0UL);
    CircularBuffer::CircularBuffer<Item40, std::dynamic_extent> small(std::span<Item40>{});
    CircularBuffer::CircularBuffer<int> source(5UL);
    CircularBuffer::CircularBuffer<int> destination(std::move(source));
    CircularBuffer::CircularBuffer<int>& moved_from = source;
    EXPECT_EQ(5UL, destination.capacity());
    owned.push(1);
    owned.emplace(2);
    small.push(Item40(1UL));
    small.emplace(2UL);
    moved_from.push(3);
    moved_from.emplace(4);
    EXPECT_EQ(0UL, owned.size());
    EXPECT_EQ(0UL, small.size());
    EXPECT_EQ(0UL, moved_from.size());
    EXPECT_FALSE(owned.try_emplace(1));
    EXPECT_FALSE(small.try_emplace(1UL));
    EXPECT_FALSE(moved_from.try_emplace(1));
    EXPECT_FALSE(owned.sum().has_value());
}

TEST(CircularBufferCallerMemoryTest, CopyOfAnOwningBufferIsIndependentAndItsStatisticsWork) {
    CircularBuffer::CircularBuffer<int> source(6UL);
    for (int i = 0; i < 4; ++i) { ASSERT_TRUE(source.try_emplace(i + 10)); }
    CircularBuffer::CircularBuffer<int> copy(source);
    EXPECT_EQ(source.size(), copy.size());
    EXPECT_EQ(source.capacity(), copy.capacity());
    EXPECT_TRUE(copy.pop());
    EXPECT_EQ(4UL, source.size());
    EXPECT_EQ(3UL, copy.size());
    EXPECT_EQ(10, source.top().value());
    EXPECT_EQ(11, copy.top().value());
    EXPECT_TRUE(source.sorted().has_value());
    EXPECT_TRUE(source.median().has_value());
}

TEST(CircularBufferCallerMemoryTest, SingleThreadAnswersEqualTheOwningForm) {
    constexpr std::size_t CAPACITY = 13UL;
    constexpr std::size_t OPERATIONS = 100000UL;
    Memory<int, CAPACITY> memory;
    CircularBuffer::CircularBuffer<int> owned(CAPACITY);
    CircularBuffer::CircularBuffer<int, std::dynamic_extent> borrowed(memory.exact());
    std::mt19937 engine(20261126U);
    std::uniform_int_distribution<int> which(0, 9);
    std::uniform_int_distribution<int> value(-50, 50);
    for (std::size_t step = 0UL; step < OPERATIONS; ++step) {
        const int choice = which(engine);
        const int x = value(engine);
        switch (choice) {
            case 0: case 1: case 2: case 3:
                ASSERT_EQ(owned.try_emplace(x), borrowed.try_emplace(x)) << "step " << step;
                break;
            case 4: case 5: {
                int a = 0, b = 0;
                const bool ra = owned.consume([&a](const int& item) { a = item; });
                const bool rb = borrowed.consume([&b](const int& item) { b = item; });
                ASSERT_EQ(ra, rb) << "step " << step;
                ASSERT_EQ(a, b) << "step " << step;
                break;
            }
            case 6:
                ASSERT_EQ(owned.pop(), borrowed.pop()) << "step " << step;
                break;
            case 7:
                owned.push(x);
                borrowed.push(x);
                break;
            default:
                break;
        }// end switch (choice)
        ASSERT_EQ(owned.top(), borrowed.top()) << "step " << step;
        ASSERT_EQ(owned.last(), borrowed.last()) << "step " << step;
        ASSERT_EQ(owned.size(), borrowed.size()) << "step " << step;
        ASSERT_EQ(owned.empty(), borrowed.empty()) << "step " << step;
        ASSERT_EQ(owned.sum(), borrowed.sum()) << "step " << step;
        ASSERT_EQ(owned.mean(), borrowed.mean()) << "step " << step;
        ASSERT_EQ(owned.minimum(), borrowed.minimum()) << "step " << step;
        ASSERT_EQ(owned.maximum(), borrowed.maximum()) << "step " << step;
    }// end for (step)
}

template <typename Buffer>
std::pair<std::uint64_t, bool> transfer_one_to_one(Buffer& buffer, const std::uint64_t& count) {
    std::atomic<bool> failed{false};
    std::uint64_t checksum = 0UL;
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(120);
    std::thread producer([&buffer, &failed, &deadline, count]() {
        for (std::uint64_t i = 0UL; i < count; ++i) {
            while (!buffer.try_emplace(Item40(i))) {
                if (failed.load(std::memory_order_relaxed) || std::chrono::steady_clock::now() > deadline) {
                    failed.store(true);
                    return;
                }// end if (failed || late)
                std::this_thread::yield();
            }// end while (!buffer.try_emplace(Item40(i)))
        }// end for (i)
    });
    bool in_order = true;
    std::thread consumer([&buffer, &failed, &deadline, &checksum, &in_order, count]() {
        std::uint64_t expected = 0UL;
        while (expected < count) {
            const bool got = buffer.consume([&checksum, &in_order, &expected](const Item40& item) {
                if (item.v[0] != expected || item.v[4] != expected + 4UL) {
                    in_order = false;
                }// end if (item is not the expected one)
                checksum += item.v[0] * 3UL + item.v[4];
                ++expected;
            });
            if (!got) {
                if (failed.load(std::memory_order_relaxed) || std::chrono::steady_clock::now() > deadline) {
                    failed.store(true);
                    return;
                }// end if (failed || late)
                std::this_thread::yield();
            }// end if (!got)
        }// end while (expected < count)
    });
    producer.join();
    consumer.join();
    return {checksum, in_order && !failed.load()};
}// end std::pair<std::uint64_t, bool> transfer_one_to_one(Buffer& buffer, const std::uint64_t& count)

TEST(CircularBufferCallerMemoryTest, OneProducerOneConsumerOnAllThreeForms) {
    constexpr std::size_t CAPACITY = 64UL;
    constexpr std::uint64_t COUNT = 1000000UL;
    Memory<Item40, CAPACITY> memory;
    CircularBuffer::CircularBuffer<Item40> owned(CAPACITY);
    CircularBuffer::CircularBuffer<Item40, CAPACITY> fixed;
    CircularBuffer::CircularBuffer<Item40, std::dynamic_extent> borrowed(memory.exact());
    const std::pair<std::uint64_t, bool> from_owned    = transfer_one_to_one(owned, COUNT);
    const std::pair<std::uint64_t, bool> from_fixed    = transfer_one_to_one(fixed, COUNT);
    const std::pair<std::uint64_t, bool> from_borrowed = transfer_one_to_one(borrowed, COUNT);
    EXPECT_TRUE(from_owned.second);
    EXPECT_TRUE(from_fixed.second);
    EXPECT_TRUE(from_borrowed.second);
    EXPECT_EQ(from_owned.first, from_borrowed.first);
    EXPECT_EQ(from_owned.first, from_fixed.first);
    EXPECT_TRUE(owned.empty());
    EXPECT_TRUE(fixed.empty());
    EXPECT_TRUE(borrowed.empty());
}
