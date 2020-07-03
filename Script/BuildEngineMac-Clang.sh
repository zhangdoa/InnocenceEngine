#!/bin/sh
cd ../
mkdir Build
cd Build

export PATH="/usr/local/opt/llvm/bin:$PATH"
export LDFLAGS="-L/usr/local/opt/llvm/Lib"
export CPPFLAGS="-I/usr/local/opt/llvm/Include"
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_GAME=ON -G "Unix Makefiles" -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ../Source
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_GAME=ON -G "Unix Makefiles" -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ../Source
make
