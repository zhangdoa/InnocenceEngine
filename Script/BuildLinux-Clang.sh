#!/bin/sh
cd ../
mkdir Build
cd Build
cmake -DINNO_PLATFORM_LINUX=ON -DBUILD_GAME=ON -DCMAKE_C_COMPILER=clang-6.0 -DCMAKE_CXX_COMPILER=clang++-6.0 -G "Unix Makefiles" ../Source
cmake -DINNO_PLATFORM_LINUX=ON -DBUILD_GAME=ON -DCMAKE_C_COMPILER=clang-6.0 -DCMAKE_CXX_COMPILER=clang++-6.0 -G "Unix Makefiles" ../Source
make
