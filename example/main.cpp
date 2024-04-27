#include <iostream>
#include <span>
#include <thread>
#include <chrono>

#include "CircularBuffer.hpp"

constexpr size_t performance_size = 1000000UL;

void producer(CircularBuffer::CircularBuffer<size_t>& buffer) {
    for (size_t i = 0; i < 10; ++i) {
        buffer.push(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void consumer(CircularBuffer::CircularBuffer<size_t>& buffer) {
    while (true) {
        if (!buffer.empty()) {
            auto item = buffer.top_pop();
            if (item) {
                std::cout << "Consumed: " << *item << std::endl;
                if (*item == buffer.size()) break;
            }
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

        // Simulating buffer use
        for (size_t i = 0; i < 15; ++i) {
            buffer.push(i % 10);  // Wrap around after 10
        }

        // Try to obtain a span from the buffer
        auto data_span = buffer.span();
        if (data_span.has_value()) {
            for (auto& val : *data_span) {
                std::cout << val << " ";
            }
        }
        std::cout << std::endl;
    }
    {
        CircularBuffer::CircularBuffer<int> buffer(10);  // Starts dynamic but with initial capacity

        for (size_t i = 0; i < 15; ++i) {
            buffer.push(i);  // Automatically handles resizing if necessary
        }

        auto data_span = buffer.span();
        if (data_span.has_value()) {
            for (auto val : *data_span) {
                std::cout << val << " ";
            }
        }
        std::cout << std::endl;

    }
    {
        CircularBuffer::CircularBuffer<int> buffer(10);

        std::thread prod(producer, std::ref(buffer));
        std::thread cons(consumer, std::ref(buffer));

        prod.join();
        cons.join();
    }
    {
        CircularBuffer::CircularBuffer<int> buffer(10);

        std::thread prod(producer, std::ref(buffer));
        std::thread cons(consumer, std::ref(buffer));

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