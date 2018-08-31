#!/bin/sh
mkdir build
cd build
cmake -DINNO_PLATFORM_MACOS=ON -DINNO_RENDERER_OPENGL=ON -DBUILD_GAME=ON -G "Xcode" ../source
cmake -DINNO_PLATFORM_MACOS=ON -DINNO_RENDERER_OPENGL=ON -DBUILD_GAME=ON -G "Xcode" ../source

