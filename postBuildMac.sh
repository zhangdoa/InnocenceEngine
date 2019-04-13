#!/bin/sh
# copy binary
rm -rf bin
mkdir bin
cp -r build/lib/* build/InnoMain/Debug/
cp -r build/InnoMain/Debug/* bin/

# copy for sandbox
rm -rf $HOME/Library/Containers/InnocenceEngine.InnoMain/Data/res
mkdir $HOME/Library/Containers/InnocenceEngine.InnoMain/Data/res
cp -r res/* $HOME/Library/Containers/InnocenceEngine.InnoMain/Data/res/
