#!/bin/sh
cd ../
mkdir Build
cd Build
cmake -DBUILD_GAME=ON -G "Unix Makefiles" ../Source
cmake -DBUILD_GAME=ON -G "Unix Makefiles" ../Source
make