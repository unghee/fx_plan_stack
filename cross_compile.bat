rd  /s /q build
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=CMAKE_TOOLCHAIN_FILE
make
cd ..