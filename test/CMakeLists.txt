# If not found, fetch it from the repository
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE # Do a shallow clone to speed up the process
    )

    # This line ensures GoogleTest uses the same runtime library
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(googletest)
    add_library(GTest::GTest ALIAS gtest)
    add_library(GTest::Main ALIAS gtest_main)
endif()


# Define the GTest package location using Homebrew
set(GTEST_ROOT /usr/local/opt/googletest)
set(GTEST_INCLUDE_DIRS ${GTEST_ROOT}/include)
set(GTEST_LIB_DIR ${GTEST_ROOT}/lib)

# Define the GTest libraries
set(GTEST_LIBRARIES ${GTEST_LIB_DIR}/libgtest.a ${GTEST_LIB_DIR}/libgtest_main.a)

# Add the include directories and link libraries
include_directories(${GTEST_INCLUDE_DIRS})


macro(package_add_test TESTNAME)
    add_executable(${TESTNAME} ${ARGN})
    target_include_directories(${TESTNAME} PRIVATE ${GTEST_INCLUDE_DIRS})
    target_link_libraries(${TESTNAME} PRIVATE ${GTEST_LIBRARIES} ${PROJECT_NAME})
    add_test(NAME ${TESTNAME} COMMAND ${TESTNAME})
    set_target_properties(${TESTNAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testbin
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testlib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testlib
    )
    add_dependencies(${TESTNAME} ${PROJECT_NAME})
endmacro()


# # Include GTest directories and link libraries
# include_directories(${GTest_INCLUDE_DIRS})
# set(GTEST_LIBRARIES GTest::GTest GTest::Main)

# macro(package_add_test TESTNAME)
#     add_executable(${TESTNAME} ${ARGN})
#     target_include_directories(${TESTNAME} PRIVATE ${GTest_INCLUDE_DIRS})
#     target_link_libraries(${TESTNAME} PRIVATE ${GTEST_LIBRARIES} ${PROJECT_NAME})
#     add_test(NAME ${TESTNAME} COMMAND ${TESTNAME})
#     set_target_properties(${TESTNAME} PROPERTIES
#         RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testbin
#         ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testlib
#         LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testlib
#     )
#     add_dependencies(${TESTNAME} ${PROJECT_NAME})
# endmacro()

# Add the tests
package_add_test(CircularBufferFixedTest CircularBufferFixedTest.cpp)
package_add_test(CircularBufferDynamicTest CircularBufferDynamicTest.cpp)
package_add_test(CircularBufferTest CircularBufferTest.cpp)
