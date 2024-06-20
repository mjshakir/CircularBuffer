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
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CircularBufferPushInt)->RangeMultiplier(2)->Range(100, 10000)->Complexity();

// Benchmark pop operation for integers
static void BM_CircularBufferPopInt(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<int, 100> buffer;
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        buffer.pop();
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CircularBufferPopInt)->RangeMultiplier(2)->Range(100, 10000)->Complexity();

// Benchmark push operation for strings
static void BM_CircularBufferPushString(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<std::string, 100> buffer;
    for (auto _ : state) {
        buffer.push(generate_random_string());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CircularBufferPushString)->RangeMultiplier(2)->Range(100, 10000)->Complexity();

// Benchmark pop operation for strings
static void BM_CircularBufferPopString(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<std::string, 100> buffer;
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_string());
    }
    for (auto _ : state) {
        buffer.pop();
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CircularBufferPopString)->RangeMultiplier(2)->Range(100, 10000)->Complexity();

// Benchmark sum method for integers
static void BM_CircularBufferSumInt(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<int, 100> buffer;
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(buffer.sum());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CircularBufferSumInt)->RangeMultiplier(2)->Range(100, 10000)->Complexity();

// Benchmark mean method for integers
static void BM_CircularBufferMeanInt(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<int, 100> buffer;
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(buffer.mean());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CircularBufferMeanInt)->RangeMultiplier(2)->Range(100, 10000)->Complexity();

// Benchmark variance method for integers
static void BM_CircularBufferVarianceInt(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<int, 100> buffer;
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(buffer.variance());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CircularBufferVarianceInt)->RangeMultiplier(2)->Range(100, 10000)->Complexity();

// Benchmark standard deviation method for integers
static void BM_CircularBufferStdDevInt(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<int, 100> buffer;
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(buffer.standard_deviation());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CircularBufferStdDevInt)->RangeMultiplier(2)->Range(100, 10000)->Complexity();

// Benchmark minimum method for integers
static void BM_CircularBufferMinInt(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<int, 100> buffer;
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(buffer.minimum());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CircularBufferMinInt)->RangeMultiplier(2)->Range(100, 10000)->Complexity();

// Benchmark maximum method for integers
static void BM_CircularBufferMaxInt(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<int, 100> buffer;
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(buffer.maximum());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CircularBufferMaxInt)->RangeMultiplier(2)->Range(100, 10000)->Complexity();

// Benchmark sorted method for integers
static void BM_CircularBufferSortedInt(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<int, 100> buffer;
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(buffer.sorted());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CircularBufferSortedInt)->RangeMultiplier(2)->Range(100, 10000)->Complexity();

// Benchmark reverse sorted method for integers
static void BM_CircularBufferReverseSortedInt(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<int, 100> buffer;
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(buffer.reverse_sorted());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CircularBufferReverseSortedInt)->RangeMultiplier(2)->Range(100, 10000)->Complexity();

// Benchmark median method for integers
static void BM_CircularBufferMedianInt(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<int, 100> buffer;
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(buffer.median());
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CircularBufferMedianInt)->RangeMultiplier(2)->Range(100, 10000)->Complexity();

// Benchmark iteration over the buffer for integers
static void BM_CircularBufferIterateInt(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<int, 100> buffer;
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_int());
    }
    for (auto _ : state) {
        for (auto it = buffer.begin(); it != buffer.end(); ++it) {
            benchmark::DoNotOptimize(*it);
        }
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CircularBufferIterateInt)->RangeMultiplier(2)->Range(100, 10000)->Complexity();

// Benchmark iteration over the buffer for strings
static void BM_CircularBufferIterateString(benchmark::State& state) {
    CircularBuffer::CircularBufferFixed<std::string, 100> buffer;
    for (int i = 0; i < state.range(0); ++i) {
        buffer.push(generate_random_string());
    }
    for (auto _ : state) {
        for (auto it = buffer.begin(); it != buffer.end(); ++it) {
            benchmark::DoNotOptimize(*it);
        }
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_CircularBufferIterateString)->RangeMultiplier(2)->Range(100, 10000)->Complexity();

BENCHMARK_MAIN();