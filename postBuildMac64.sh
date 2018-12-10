#!/bin/sh
rm -rf bin
mkdir bin
cp -r build/bin/* bin/
rm -rf build/res/
mkdir build/res
cp -r res/* build/res/
