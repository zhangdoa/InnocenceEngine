#!/bin/sh
git submodule update

cd source/external/gitsubmodules
cp -R assimp/include/assimp/ ../include/assimp/

cp -R Vulkan-Headers/include/vulkan/ ../include/vulkan/

cp -R glfw/include/GLFW/ ../include/GLFW/

mkdir -p ../include/stb
cp  stb/stb_image.h ../include/stb/

cd ../

mkdir -p dll/macos
mkdir -p lib/macos
