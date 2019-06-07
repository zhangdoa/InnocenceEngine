#!/bin/sh
cd ../Source/External/GitSubmodules/GLAD

mkdir Build
cd Build
cmake -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF -G "Xcode" ../
xcodebuild build

cp -r include/ ../../../Include/
cp Debug/*.a ../../../Lib/Mac/
