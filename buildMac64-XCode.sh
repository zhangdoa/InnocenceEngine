#!/bin/sh
mkdir build
cd build
cmake -DINNO_PLATFORM_MACOS=ON -G "Xcode" ../source
cmake -DINNO_PLATFORM_MACOS=ON -G "Xcode" ../source

