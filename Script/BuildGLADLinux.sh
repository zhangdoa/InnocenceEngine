#!/bin/sh
cd ../Source/External/GitSubmodules/GLAD/

mkdir Build
cd Build
mkdir Debug
cd Debug
cmake -DCMAKE_C_FLAGS="-fPIC" -G "Unix Makefiles" ../../
make

cp -r include/* ../../../../Include/

cp *.a ../../../../Lib/Linux/
