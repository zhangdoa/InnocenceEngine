#!/bin/sh
rm -rf bin
rm -rf build/InnoMain/build/Debug/res/

mkdir bin
cp -r build/lib/* build/InnoMain/build/Debug/
cp -r build/InnoMain/build/Debug/* bin/

mkdir build/InnoMain/build/Debug/res/
cp -r res/* build/InnoMain/build/Debug/res/
