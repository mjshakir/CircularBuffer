cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
#------------------------------------------------------------------------------------------
# Determine if CircularBuffer is built as a standalone project or included by other projects
set(CIRCULARBUFFER_STANDALONE_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CIRCULARBUFFER_STANDALONE_PROJECT ON)
endif()
#------------------------------------------------------------------------------------------
# Get the name of the folder and use it as the project name
get_filename_component(PARENT_DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" PARENT_DIR_NAME ${PARENT_DIR_NAME})
project(${PARENT_DIR_NAME} VERSION 1.0 DESCRIPTION "A Circularbuffer function library" LANGUAGES CXX)
#------------------------------------------------------------------------------------------
# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#------------------------------------------------------------------------------------------
# Print out the build type
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
#------------------------------------------------------------------------------------------
if(CIRCULARBUFFER_STANDALONE_PROJECT)
    if(MSVC)
        # MSVC specific warning flags
        add_library(common_options INTERFACE)
        target_compile_options(common_options INTERFACE /W4)  # Equivalent to -Wall in GCC/Clang
    else()
        # GCC/Clang specific warning flags
        add_library(common_options INTERFACE)
        target_compile_options(common_options INTERFACE -Wall -Wextra -Wpedantic -pedantic-errors -Werror=format-security -fasynchronous-unwind-tables)
    endif()
endif()
#------------------------------------------------------------------------------------------
# Set the runtime output directory only if this is a standalone project
if(CIRCULARBUFFER_STANDALONE_PROJECT)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    # Optionally, to set these directories only for specific build configurations (Debug, Release, etc.),
    # you can use the following approach:
    # set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
    # set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
    # set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
    # set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
    # set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
    # set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
    # Diagnostic output
    message(STATUS "ARCHIVE_OUTPUT_DIRECTORY: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
    message(STATUS "LIBRARY_OUTPUT_DIRECTORY: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    message(STATUS "RUNTIME_OUTPUT_DIRECTORY: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()
#------------------------------------------------------------------------------------------
# Offer switches for the type of the library and optional builds
option(BUILD_CIRCULARBUFFER_SHARED_LIBS "Build using shared libraries" ON)
option(BUILD_CIRCULARBUFFER_EXAMPLE "Build CircularBuffer example" ${CIRCULARBUFFER_STANDALONE_PROJECT})
option(BUILD_CIRCULARBUFFER_TESTS "Build CircularBuffer tests" ${CIRCULARBUFFER_STANDALONE_PROJECT})
option(BUILD_CIRCULARBUFFER_BENCHMARK "Build CircularBuffer benchmarks" ${CIRCULARBUFFER_STANDALONE_PROJECT})
option(BUILD_CIRCULARBUFFER_MULTI_THREADING "Enable multi-threading support." OFF)
set(HAS_TBB OFF CACHE BOOL "Set to TRUE if TBB is found, otherwise FALSE")
#------------------------------------------------------------------------------------------
# Force colored output
option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." OFF)
if(${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_compile_options(-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options(-fcolor-diagnostics)
    endif()
endif()
#------------------------------------------------------------------------------------------
# Define the source and include directories
set(CIRCULARBUFFER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(CIRCULARBUFFER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
#------------------------------------------------------------------------------------------
# Find TBB
find_package(TBB QUIET)
if(TBB_FOUND)
    set(HAS_TBB ON)
    message(STATUS "Intel TBB found: ${TBB_VERSION}")
else()
    set(HAS_TBB OFF)
endif()
#------------------------------------------------------------------------------------------
# Define the source files
set(CIRCULARBUFFER_SOURCES
    ${CIRCULARBUFFER_SOURCE_DIR}/CircularBufferFixed.cpp
    ${CIRCULARBUFFER_SOURCE_DIR}/CircularBufferDynamic.cpp
    ${CIRCULARBUFFER_SOURCE_DIR}/CircularBuffer.cpp
)
#------------------------------------------------------------------------------------------
# Check if the library should be built as a shared library
if(BUILD_CIRCULARBUFFER_SHARED_LIBS)
    add_library(${PROJECT_NAME} SHARED ${CIRCULARBUFFER_SOURCES})
    # If needed, set properties specific to shared libraries here
else()
    add_library(${PROJECT_NAME} STATIC ${CIRCULARBUFFER_SOURCES})
    # If needed, set properties specific to static libraries here
endif()
#------------------------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PUBLIC 
    $<BUILD_INTERFACE:${CIRCULARBUFFER_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
#------------------------------------------------------------------------------------------
# Alias for CircularBuffer library
add_library(CircularBuffer::circularbuffer ALIAS ${PROJECT_NAME})
#------------------------------------------------------------------------------------------
# Set CIRCULARBUFFER_LIBRARIES variable
if(CIRCULARBUFFER_STANDALONE_PROJECT)
    set(CIRCULARBUFFER_LIBRARIES CircularBuffer::circularbuffer)
else()
    set(CIRCULARBUFFER_LIBRARIES CircularBuffer::circularbuffer PARENT_SCOPE)
endif()
message(STATUS "CIRCULARBUFFER_LIBRARIES: ${CIRCULARBUFFER_LIBRARIES}")
#------------------------------------------------------------------------------------------
# Conditional compilation and warnings
if(BUILD_CIRCULARBUFFER_MULTI_THREADING)
    if(HAS_TBB)
        message(STATUS "Multi-threading enabled with Intel TBB.")
        target_link_libraries(${PROJECT_NAME} PRIVATE TBB::tbb)
    else()
        message(WARNING "Multi-threading is enabled, but Intel TBB is not found. Compiling without multi-threading support.")
    endif()
endif()
#------------------------------------------------------------------------------------------
# Building examples
if(BUILD_CIRCULARBUFFER_EXAMPLE)
    set(CIRCULARBUFFER_EXAMPLE_NAME "${PROJECT_NAME}_example")
    add_executable(${CIRCULARBUFFER_EXAMPLE_NAME} example/main.cpp)
    target_link_libraries(${CIRCULARBUFFER_EXAMPLE_NAME} PRIVATE ${PROJECT_NAME})
endif()
#------------------------------------------------------------------------------------------
# Compiler-specific flags
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /MD")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /MDd")
    target_compile_options(${PROJECT_NAME} PRIVATE 
        "$<$<CONFIG:Release>:/O2>" 
        "$<$<CONFIG:Debug>:/Od>" 
        "$<$<CONFIG:Debug>:/Zi>"
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_options(${PROJECT_NAME} PRIVATE /DEBUG)
    endif()

    set_target_properties(${PROJECT_NAME} PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    )
else()
    # For non-MSVC compilers
    target_compile_options(${PROJECT_NAME} PRIVATE 
        "$<$<CONFIG:Release>:-O3>"
    )
    # Conditionally include AddressSanitizer flags only if standalone and in Debug mode
    if(CIRCULARBUFFER_STANDALONE_PROJECT AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang|GNU")
        set(SANITIZE_FLAGS "$<$<CONFIG:Debug>:-fsanitize=address>")
        target_compile_options(${PROJECT_NAME} PRIVATE 
            "$<$<CONFIG:Debug>:-g>"
            "$<$<CONFIG:Debug>:-O0>"
            ${SANITIZE_FLAGS}
        )
        target_link_options(${PROJECT_NAME} PRIVATE ${SANITIZE_FLAGS})
    endif()
endif()
#------------------------------------------------------------------------------------------
# Enable testing and add the test directory if this is a standalone project
if(BUILD_CIRCULARBUFFER_TESTS AND CIRCULARBUFFER_STANDALONE_PROJECT)
    enable_testing()
    add_subdirectory(test)
endif()
#------------------------------------------------------------------------------------------
# Enable benchmarks and add the benchmark directory if this is a standalone project
if(BUILD_CIRCULARBUFFER_BENCHMARK AND CIRCULARBUFFER_STANDALONE_PROJECT)
    add_subdirectory(benchmark)
endif()
#------------------------------------------------------------------------------------------
# Export the library targets (only if it's a standalone project)
if(CIRCULARBUFFER_STANDALONE_PROJECT)
    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME} EXPORT CircularBufferTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY ${CIRCULARBUFFER_INCLUDE_DIR}/${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(EXPORT CircularBufferTargets
        FILE CircularBufferTargets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )
endif()
#------------------------------------------------------------------------------------------