#!/bin/sh
mkdir build
cd build
export LDFLAGS="-L/usr/local/opt/llvm/lib"
export CPPFLAGS="-I/usr/local/opt/llvm/include"
cmake -DINNO_PLATFORM_MACOS=ON -G "Unix Makefiles" -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ../source
cmake -DINNO_PLATFORM_MACOS=ON -G "Unix Makefiles" -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ../source
make
