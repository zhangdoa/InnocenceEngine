#!/bin/sh
cd ../Source/External/GitSubmodules

cd assimp
git checkout master
git pull
cd ../

cd GLAD
git checkout master
git pull
cd ../

cd imgui
git checkout master
git pull
cd ../

cd json
git checkout master
git pull
cd ../

cd PhysX
git checkout 4.0
git pull
cd ../

cd QDarkStyleSheet
git checkout master
git pull
cd ../

cd stb
git checkout master
git pull
cd ../

cd Vulkan-Headers
git checkout master
git pull
cd ../