#!/bin/sh
mkdir build
cd build
cmake -DINNO_PLATFORM_LINUX64=ON -DBUILD_GAME=ON -G "CodeBlocks - Unix Makefiles" ../source
cmake -DINNO_PLATFORM_LINUX64=ON -DBUILD_GAME=ON -G "CodeBlocks - Unix Makefiles" ../source

