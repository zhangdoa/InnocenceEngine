#!/bin/sh
mkdir build
cd build
cmake -DINNO_PLATFORM_WIN32=OFF -DINNO_PLATFORM_WIN64=OFF -DINNO_PLATFORM_LINUX64=ON -G "CodeBlocks - Unix Makefiles" ../source
cmake -DINNO_PLATFORM_WIN32=OFF -DINNO_PLATFORM_WIN64=OFF -DINNO_PLATFORM_LINUX64=ON -G "CodeBlocks - Unix Makefiles" ../source

