#!/bin/bash

# Exit this script immediately upon any error.
set -e

echo "Plan stack builder at your service :)"

rm -rf build
mkdir -p build
cd build
if  [[ $1 = "-pi" ]]; then
	cmake .. -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=CMAKE_RASPBERRY_PI_TOOLCHAIN_FILE
else
	cmake .. -G "Ninja"
fi
ninja
cd ..

# guest host OS
if [[ $1 = "-pi" ]]; then
	HOST_OS="raspberryPi"
elif [[ "$OSTYPE" == "linux-gnu" ]]; then
	HOST_OS="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
	HOST_OS="mac"
elif [[ "$OSTYPE" == "cygwin" ]]; then
	HOST_OS="windows"
elif [[ "$OSTYPE" == "msys" ]]; then
	HOST_OS="windows"
else
	HOST_OS="linux"
fi

cp build/libs/* libs/${HOST_OS}

exit 0
