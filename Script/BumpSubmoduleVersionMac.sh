#!/bin/sh
cd ../Source/External/GitSubmodules

cd assimp
git checkout v5.2.5
cd ../

cd GLAD
git checkout v2.0.4
git pull
cd ../

cd imgui
git checkout v1.89.4
git pull
cd ../

cd json
git checkout v3.11.2
git pull
cd ../

cd PhysX
git checkout 4.0
git pull
cd ../

cd QDarkStyleSheet
git checkout v3.1
git pull
cd ../

cd stb
git checkout master
git pull
cd ../

cd Vulkan-Headers
git checkout v1.3.245
git pull
cd ../