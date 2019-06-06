#!/bin/sh
cd ../Source/External/GitSubmodules/assimp

mkdir build
cd build
cmake -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF -G "Unix Makefiles" ../
make

cp include/assimp/config.h ../../../Include/assimp/
cp code/*.so ../../../Lib/Linux
