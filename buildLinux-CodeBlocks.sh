#!/bin/sh
mkdir build
cd build
cmake -DINNO_PLATFORM_LINUX=ON -G "CodeBlocks - Unix Makefiles" ../source
cmake -DINNO_PLATFORM_LINUX=ON -G "CodeBlocks - Unix Makefiles" ../source

