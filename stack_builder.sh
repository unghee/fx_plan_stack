#!/bin/sh

rm -rf build
mkdir -p build
cd build
cmake .. -G "Eclipse CDT4 - Ninja"
ninja
cd ..