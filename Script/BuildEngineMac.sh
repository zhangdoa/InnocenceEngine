#!/bin/sh
cd ../
mkdir Build
cd Build
export LDFLAGS="-L/usr/local/opt/llvm/Lib"
export CPPFLAGS="-I/usr/local/opt/llvm/Include"
cmake -DBUILD_GAME=ON -G "Unix Makefiles" -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ../Source
cmake -DBUILD_GAME=ON -G "Unix Makefiles" -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ../Source
make
cd ../Source/Engine/Platform/MacMain
xcodebuild build SYMROOT=../../../../Build/InnoMain -configuration Debug