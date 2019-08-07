#!/bin/bash

# Exit this script immediately upon any error.
set -e

echo "Plan stack builder at your service :)"

rm -rf build
mkdir -p build
cd build
cmake .. -G "Ninja"
ninja
cd ..

# guest host OS
if [[ "$OSTYPE" == "linux-gnu" ]]; then
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
