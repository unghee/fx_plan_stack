#!/bin/sh

rm -rf build
mkdir -p build
cd build
cmake .. -G "Eclipse CDT4 - Ninja" -DCMAKE_TOOLCHAIN_FILE=MINGW7-3_X86_TOOLCHAIN_FILE
ninja
cd ..
