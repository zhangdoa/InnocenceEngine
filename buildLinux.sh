#!/bin/sh
mkdir build
cd build
cmake -DINNO_PLATFORM_LINUX=ON -G "Unix Makefiles" ../source
cmake -DINNO_PLATFORM_LINUX=ON -G "Unix Makefiles" ../source
make
