#!/bin/sh
cd ../
mkdir Build
cd Build
cmake -DINNO_PLATFORM_LINUX=ON -DBUILD_GAME=ON -G "CodeBlocks - Unix Makefiles" ../Source
cmake -DINNO_PLATFORM_LINUX=ON -DBUILD_GAME=ON -G "CodeBlocks - Unix Makefiles" ../Source
