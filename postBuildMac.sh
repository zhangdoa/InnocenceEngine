#!/bin/sh
rm -rf bin
rm -rf $HOME/Library/Containers/InnocenceEngine.InnoMain/Data/res

mkdir bin
cp -r build/lib/* build/InnoMain/Debug/
cp -r build/InnoMain/Debug/* bin/

mkdir $HOME/Library/Containers/InnocenceEngine.InnoMain/Data/res
cp -r res/* $HOME/Library/Containers/InnocenceEngine.InnoMain/Data/res
