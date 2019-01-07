#!/bin/sh
cd source/external/gitsubmodules/glfw

mkdir build_dll
cd build_dll
cmake -DBUILD_SHARED_LIBS=ON -G "Xcode" ../
xcodebuild build

cp  src/Debug/*.dylib ../../../lib/macos/
