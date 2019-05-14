#!/bin/sh

rm -rf build
mkdir -p build
cd build
cmake .. -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=CMAKE_RASPBERRY_PI_TOOLCHAIN_FILE #MINGW7-3_X86_TOOLCHAIN_FILE
ninja
cd ..
