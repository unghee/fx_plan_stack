#!/bin/bash

echo Checking out branch "$1" of all submodules
git submodule foreach --recursive git checkout "$1"
git submodule foreach --recursive git pull