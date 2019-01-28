rd  /s /q build
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=CMAKE_RASPBERRY_PI_TOOLCHAIN_FILE
make
cd ..