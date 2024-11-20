# Try Finding Installed Google Test
find_package(GTest QUIET)
#------------------------------------------------------------------------------------------
# Use Installed Version or Fetch
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
#------------------------------------------------------------------------------------------
# Function to setup a test executable
function(create_test_target TARGET_NAME SOURCE_FILE)
    # Correcting the file path according to the directory structure
    add_executable(${TARGET_NAME} ${SOURCE_FILE})
    target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
    
    # Set the output directories for test executables
    set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testbin
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testlib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testlib
    )
    
    # Link the main project library and Google Test
    target_link_libraries(${TARGET_NAME} PRIVATE ${PROJECT_NAME})
    if(GTest_FOUND)
        if(HAS_TBB)
            target_link_libraries(${TARGET_NAME} PRIVATE GTest::GTest GTest::Main tbb)
        else()
            target_link_libraries(${TARGET_NAME} PRIVATE GTest::GTest GTest::Main)
        endif()
    else()
        if(HAS_TBB)
            target_link_libraries(${TARGET_NAME} PRIVATE gtest gtest_main tbb)
        else()
            target_link_libraries(${TARGET_NAME} PRIVATE gtest gtest_main)
        endif()
    endif()

    # Enable testing and add the test to CTest
    enable_testing()
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
endfunction()
#------------------------------------------------------------------------------------------
# Add the tests without the 'test/' prefix since we are already in the test directory
create_test_target(${PROJECT_NAME}_Fixed_Test   CircularBufferFixedTest.cpp)
create_test_target(${PROJECT_NAME}_Dynamic_Test CircularBufferDynamicTest.cpp)
create_test_target(${PROJECT_NAME}_Test         CircularBufferTest.cpp)
#------------------------------------------------------------------------------------------