#!/bin/sh
mkdir build
cd build
cmake -DINNO_PLATFORM_LINUX64=ON -DINNO_RENDERER_OPENGL=ON -DBUILD_GAME=ON -G "Unix Makefiles" ../source
cmake -DINNO_PLATFORM_LINUX64=ON -DINNO_RENDERER_OPENGL=ON -DBUILD_GAME=ON -G "Unix Makefiles" ../source

