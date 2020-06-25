#!/bin/sh

mkdir build
cd build

cmake ..

cmake --build .

#ctest .
./test_ddoheap
