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
    CircularBuffer::CircularBuffer<int> buffer(state.range(0));
    for (auto _ : state) {
        buffer.push(generate_random_int());
    }
}
BENCHMARK(BM_CircularBufferPushInt)->Arg(100)->Arg(1000)->Arg(10000);

// Benchmark pop operation for integers
static void BM_CircularBufferPopInt(benchmark::State& state) {
    CircularBuffer::CircularBuffer<int> buffer(state.range(0));
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        buffer.pop();
    }
}
BENCHMARK(BM_CircularBufferPopInt)->Arg(100)->Arg(1000)->Arg(10000);

// Benchmark push operation for strings
static void BM_CircularBufferPushString(benchmark::State& state) {
    CircularBuffer::CircularBuffer<std::string> buffer(state.range(0));
    for (auto _ : state) {
        buffer.push(generate_random_string());
    }
}
BENCHMARK(BM_CircularBufferPushString)->Arg(100)->Arg(1000)->Arg(10000);

// Benchmark pop operation for strings
static void BM_CircularBufferPopString(benchmark::State& state) {
    CircularBuffer::CircularBuffer<std::string> buffer(state.range(0));
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_string());
    }
    for (auto _ : state) {
        buffer.pop();
    }
}
BENCHMARK(BM_CircularBufferPopString)->Arg(100)->Arg(1000)->Arg(10000);

// Benchmark statistical methods for integers
static void BM_CircularBufferStatsInt(benchmark::State& state) {
    CircularBuffer::CircularBuffer<int> buffer(state.range(0));
    for (int i = 0; i < state.range(0); ++i) {
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
BENCHMARK(BM_CircularBufferStatsInt)->Arg(100)->Arg(1000)->Arg(10000);

// Benchmark iteration over the buffer for integers
static void BM_CircularBufferIterateInt(benchmark::State& state) {
    CircularBuffer::CircularBuffer<int> buffer(state.range(0));
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        for (auto it = buffer.begin(); it != buffer.end(); ++it) {
            benchmark::DoNotOptimize(*it);
        }
    }
}
BENCHMARK(BM_CircularBufferIterateInt)->Arg(100)->Arg(1000)->Arg(10000);

// Benchmark iteration over the buffer for strings
static void BM_CircularBufferIterateString(benchmark::State& state) {
    CircularBuffer::CircularBuffer<std::string> buffer(state.range(0));
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_string());
    }
    for (auto _ : state) {
        for (auto it = buffer.begin(); it != buffer.end(); ++it) {
            benchmark::DoNotOptimize(*it);
        }
    }
}
BENCHMARK(BM_CircularBufferIterateString)->Arg(100)->Arg(1000)->Arg(10000);

BENCHMARK_MAIN();