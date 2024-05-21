cmake_minimum_required(VERSION 3.10)
project(Wav VERSION 1.0 LANGUAGES CXX)
set(NAMESPACE Wav)
set(CMAKE_CXX_STANDARD 17)

option(SEGMENTER_BUILD_PYBINDINGS OFF)
add_library(${PROJECT_NAME} INTERFACE)
add_library(${NAMESPACE}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(BUILD_TESTS_WAV "Build the tests for SplitRadixFFT" OFF)

add_subdirectory(src)
if(BUILD_TESTS_WAV)
    # Add CPM
    include(cmake/CPM.cmake)
    CPMAddPackage("gh:catchorg/Catch2@3.4.0")

    enable_testing()
    add_subdirectory(tests)
endif()

