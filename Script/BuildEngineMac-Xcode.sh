#!/bin/sh
cd ../
mkdir Build
cd Build

cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_GAME=ON -G "Xcode" ../Source
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_GAME=ON -G "Xcode" ../Source
xcodebuild build -configuration Debug

cd ../Source/Engine/Platform/MacMain
xcodebuild build SYMROOT=../../../../Build/InnoMain -configuration Debug
