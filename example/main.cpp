#include <iostream>
#include <span>
#include <thread>
#include <chrono>
#include <functional>

#include "CircularBuffer.hpp"

constexpr size_t performance_size = 1000000UL;


template<typename T>
void printSpan(std::span<T> span) {
    for (auto& val : span) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
}

template<typename BufferType>
void producer(BufferType& buffer) {
    for (size_t i = 0; i < 10; ++i) {
        buffer.push(i);  // Assumes BufferType has a push method
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

template<typename BufferType>
void consumer(BufferType& buffer) {
     std::this_thread::sleep_for(std::chrono::milliseconds(200));
    while (!buffer.empty()) {
            auto item = buffer.top_pop();  // Assumes BufferType has a top_pop method
            if (item) {
                std::cout << "Consumed: " << *item << std::endl;
        }
    }
}

template<typename BufferType>
void measurePerformance(BufferType& buffer, const std::string& description) {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < performance_size; ++i) {
        buffer.push(i);
        if (performance_size % 10 == 0){
            buffer.top_pop();
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << description << " took " << elapsed.count() << " seconds.\n";
}


int main() {
    {
        CircularBuffer::CircularBuffer<size_t, 10> buffer;

        std::cout << "Fixed Test" << std::endl;

        // Simulating buffer use
        for (size_t i = 0; i < 15; ++i) {
            buffer.push(i);  // Wrap around after 10
        }

        std::cout << "Fixed Span Test: ";
        // Try to obtain a span from the buffer
        printSpan<size_t>(buffer);
        
        std::cout << "Fixed top_pop Test: ";
        while(!buffer.empty()) {
            auto value = buffer.top_pop();
            if(value.has_value()){
                std::cout << value.value() << " ";
            }
        }

        std::cout << std::endl;
    }
    {
        CircularBuffer::CircularBuffer<size_t> buffer(10);  // Starts dynamic but with initial capacity

        std::cout << "Dynamic Test: ";

        for (size_t i = 0; i < 15; ++i) {
            buffer.push(i);  // Automatically handles resizing if necessary
        }

        while(!buffer.empty()) {
            auto value = buffer.top_pop();
            if(value.has_value()){
                std::cout << value.value() << " ";
            }
        }

        std::cout << std::endl;

    }
    {
        CircularBuffer::CircularBuffer<size_t> buffer(10);

        std::cout << "Threads Dynamic Test" << std::endl;

        std::thread prod(producer<decltype(buffer)>, std::ref(buffer));
        std::thread cons(consumer<decltype(buffer)>, std::ref(buffer));

        prod.join();
        cons.join();
    }
    {
        CircularBuffer::CircularBuffer<size_t, 10> buffer;

        std::cout << "Threads Fix Test" << std::endl;

        std::thread prod(producer<decltype(buffer)>, std::ref(buffer));
        std::thread cons(consumer<decltype(buffer)>, std::ref(buffer));

        prod.join();
        cons.join();
    }
    {
        CircularBuffer::CircularBuffer<size_t, performance_size> fixedBuffer;
        CircularBuffer::CircularBuffer<size_t> dynamicBuffer(performance_size);

        measurePerformance(fixedBuffer, "Fixed buffer");
        measurePerformance(dynamicBuffer, "Dynamic buffer");
    }
    return 0;
}