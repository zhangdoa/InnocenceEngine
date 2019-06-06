#!/bin/sh
cd ../Source/External/GitSubmodules/assimp

mkdir build
cd build
cmake -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF -G "Xcode" ../
xcodebuild build

cp include/assimp/config.h ../../../Include/assimp/
cp code/Debug/*.dylib ../../../Lib/Mac/
