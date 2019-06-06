#!/bin/sh
cd ../
# copy binary
rm -rf Bin
mkdir Bin
cp -r Build/Lib/* Build/InnoMain/Debug/
cp -r Build/InnoMain/Debug/* Bin/

# copy for sandbox
rm -rf $HOME/Library/Containers/InnocenceEngine.InnoMain/Data/Res
mkdir $HOME/Library/Containers/InnocenceEngine.InnoMain/Data/Res
cp -r Res/* $HOME/Library/Containers/InnocenceEngine.InnoMain/Data/Res/
