#!/bin/sh
cd ../
rm -rf Bin
mkdir Bin
cp -r Build/Bin/* Bin/
rm -rf Build/Res/
mkdir Build/Res
cp -r Res/* Build/Res/
