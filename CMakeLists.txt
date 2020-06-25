cmake_minimum_required(VERSION 3.5)

project(ddoheap)

#### library

add_library(ddoheap STATIC ddo_heap.c)

#### executable

find_package(GTest REQUIRED)

add_executable(test_ddoheap test_ddo_heap.cpp)

target_link_libraries(test_ddoheap ddoheap GTest::Main)

#### ctest

enable_testing()

add_test(test_ddoheap "./test_ddoheap")
