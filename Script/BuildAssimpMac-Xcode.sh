#!/bin/sh
cd ../Source/External/GitSubmodules/assimp

mkdir build
cd build
cmake -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF -G "Xcode" ../
xcodebuild build -configuration Debug
xcodebuild build -configuration Release

cp code/Debug/*.dylib ../../../Lib/Mac/Debug/
cp code/Release/*.dylib ../../../Lib/Mac/Release/
