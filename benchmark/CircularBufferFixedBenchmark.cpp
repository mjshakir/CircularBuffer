#include <benchmark/benchmark.h>
#include "CircularBuffer.hpp"
#include <string>
#include <random>
#include <algorithm>

// Helper function to generate random integers
int generate_random_int() {
    static std::mt19937 generator(std::random_device{}());
    static std::uniform_int_distribution<int> distribution(1, 100);
    return distribution(generator);
}

// Helper function to generate random strings
std::string generate_random_string() {
    static std::mt19937 generator(std::random_device{}());
    static std::uniform_int_distribution<int> distribution(1, 10);
    int length = distribution(generator);
    std::string str(length, ' ');
    std::generate_n(str.begin(), length, [&]() { return 'a' + distribution(generator) % 26; });
    return str;
}

// Benchmark push operation for integers
static void BM_CircularBufferPushInt(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<int, 100> buffer;
    for (auto _ : state) {
        buffer.push(generate_random_int());
    }
}
BENCHMARK(BM_CircularBufferPushInt);

// Benchmark pop operation for integers
static void BM_CircularBufferPopInt(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<int, 100> buffer;
    for (int i = 0; i < 100; ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        buffer.pop();
    }
}
BENCHMARK(BM_CircularBufferPopInt);

// Benchmark push operation for strings
static void BM_CircularBufferPushString(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<std::string, 100> buffer;
    for (auto _ : state) {
        buffer.push(generate_random_string());
    }
}
BENCHMARK(BM_CircularBufferPushString);

// Benchmark pop operation for strings
static void BM_CircularBufferPopString(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<std::string, 100> buffer;
    for (int i = 0; i < 100; ++i) {
        buffer.push(generate_random_string());
    }
    for (auto _ : state) {
        buffer.pop();
    }
}
BENCHMARK(BM_CircularBufferPopString);

// Benchmark statistical methods for integers
static void BM_CircularBufferStatsInt(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<int, 100> buffer;
    for (int i = 0; i < 100; ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(buffer.sum());
        benchmark::DoNotOptimize(buffer.mean());
        benchmark::DoNotOptimize(buffer.variance());
        benchmark::DoNotOptimize(buffer.standard_deviation());
        benchmark::DoNotOptimize(buffer.minimum());
        benchmark::DoNotOptimize(buffer.maximum());
        benchmark::DoNotOptimize(buffer.sorted());
        benchmark::DoNotOptimize(buffer.reverse_sorted());
        benchmark::DoNotOptimize(buffer.median());
    }
}
BENCHMARK(BM_CircularBufferStatsInt);

// Benchmark iteration over the buffer for integers
static void BM_CircularBufferIterateInt(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<int, 100> buffer;
    for (int i = 0; i < 100; ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        for (auto it = buffer.begin(); it != buffer.end(); ++it) {
            benchmark::DoNotOptimize(*it);
        }
    }
}
BENCHMARK(BM_CircularBufferIterateInt);

// Benchmark iteration over the buffer for strings
static void BM_CircularBufferIterateString(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<std::string, 100> buffer;
    for (int i = 0; i < 100; ++i) {
        buffer.push(generate_random_string());
    }
    for (auto _ : state) {
        for (auto it = buffer.begin(); it != buffer.end(); ++it) {
            benchmark::DoNotOptimize(*it);
        }
    }
}
BENCHMARK(BM_CircularBufferIterateString);

BENCHMARK_MAIN();