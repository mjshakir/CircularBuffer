# CircularBuffer

`CircularBuffer` is a C++ implementation of a circular buffer that can either use a fixed-size buffer with `std::array` in a lock-free manner or a dynamic buffer with `std::deque` in a thread-safe manner. This makes it a versatile choice for different performance and usage needs.

## Build Status

| Architecture | Ubuntu | macOS | Windows |
|--------------|--------|-------|---------|
| **x86_64**   | ![Ubuntu X86_64](https://github.com/mjshakir/CircularBuffer/actions/workflows/ubuntu_X86_64.yml/badge.svg) | ![macOS X86_64](https://github.com/mjshakir/CircularBuffer/actions/workflows/macos_x86_64.yml/badge.svg) | ![Windows X86_64](https://github.com/mjshakir/CircularBuffer/actions/workflows/windows_x86_64.yml/badge.svg) |
| **ARM**      | ![Ubuntu ARM](https://github.com/mjshakir/CircularBuffer/actions/workflows/ubuntu_arm.yml/badge.svg) | ![macOS ARM](https://github.com/mjshakir/CircularBuffer/actions/workflows/macos_arm.yml/badge.svg) | ![Windows ARM](https://github.com/mjshakir/CircularBuffer/actions/workflows/windows_arm.yml/badge.svg) |
| **RISCV**    | ![Ubuntu RISCV](https://github.com/mjshakir/CircularBuffer/actions/workflows/ubuntu_riscv.yml/badge.svg) |        |         |


## Features

- **Lock-Free Implementation** using `std::array` for environments where low latency is critical.
- **Thread-Safe Implementation** using `std::deque` suitable for dynamic buffer size management.
- **Configurable** for either static or dynamic memory usage.

## Getting Started

### Prerequisites

- C++20 compiler
- CMake 3.5 or higher

### Building on Linux

#### Using make

To build the `CircularBuffer` project, you can use the following commands:

1. Clone the repository.
    ```bash
    git clone https://github.com/mjshakir/CircularBuffer.git
    ```

    ```bash
    cd CircularBuffer
    ```

2. Create a build directory and generate build system files.
    ```bash
    cmake -DFORCE_COLORED_OUTPUT=ON -DCMAKE_BUILD_TYPE=Release -B build
    ```

    ```bash
    cd build
    ```

3. Build the library.
    ```bash
    cmake --build . --config Release
    ```

4. Optionally, install the library in your system.
    ```bash
    cmake --install .
    ```

#### Using ninja

Ninja is known for its speed and is the preferred option for many developers. Follow these steps to build with Ninja:

1. Clone the repository if you haven't already.
    ```bash
    git clone https://github.com/mjshakir/CircularBuffer.git
    ```

    ```bash
    cd CircularBuffer
    ```
2. From the project root directory, generate the build files with Ninja. We enable colored output and set the build type to release for optimized code.
    ```bash
    cmake -DFORCE_COLORED_OUTPUT=ON -DCMAKE_BUILD_TYPE=Release -B build -G Ninja
    ```

3. Build the project. Since we're using Ninja, this step should be significantly faster compared to traditional methods.
    ```bash
    cd build
    ```
    ```bash
    ninja
    ```

Note: If you haven't installed Ninja, you can do so by following the instructions on [Ninja's GitHub page](https://github.com/ninja-build/ninja).

## Test The Implementation 
```bash
    ninja test # ctest
```


## Including in Another Project
You can include the `CircularBuffer` library in your CMake project by adding the following to your `CMakeLists.txt`:
```cmake
add_subdirectory(path/to/CircularBuffer)
target_link_libraries(your_target_name PRIVATE CircularBuffer::circularbuffer)
```

## Usage Example
Here's a simple example of how to use the `CircularBuffer`:

```cpp
#include <iostream>
#include "CircularBuffer.hpp"

int main() {
    CircularBuffer::CircularBuffer<int, 5> cb;  // Fixed-size circular buffer
    for (int i = 0; i < 5; ++i) {
        cb.push(i);
    }

    while (!cb.empty()) {
        std::cout << cb.front() << std::endl;
        cb.pop();
    }

    return 0;
}
```

## Contributing to CircularBuffer
We welcome contributions to `CircularBuffer`! Whether you're a seasoned developer or just starting out, there are many ways to contribute to this project.

### Ways to Contribute
1. Reporting bugs or problems: If you encounter any bugs or issues, please open an issue in the GitHub repository. We appreciate detailed reports that help us understand the cause of the problem.

2. Suggesting enhancements: Do you have ideas for making `CircularBuffer` even better? We'd love to hear your suggestions. Please create an issue to discuss your ideas.

3. Code contributions: Want to get hands-on with the code? Great! Please feel free to fork the repository and submit your pull requests. Whether it's bug fixes, new features, or improvements to documentation, all contributions are greatly appreciated.

### Contribution Process
1. Fork the repository and create your branch from main.
2. If you've added or changed code, please ensure the testing suite passes. Add new tests for new features or bug fixes.
3. Ensure your code follows the existing style to keep the codebase consistent and readable.
4. Include detailed comments in your code so others can understand your changes.
5. Write a descriptive commit message.
6. Push your branch to GitHub and submit a pull request.

## License
Distributed under the `MIT License`. See LICENSE for more information.
- Feel free to modify this README.md to better fit your project structure or specific needs.
