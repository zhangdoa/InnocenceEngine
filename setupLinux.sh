#!/bin/sh
git submodule update

cd source/external/gitsubmodules

mkdir -p ../include/assimp
cp -R assimp/include/assimp/ ../include/assimp/

mkdir -p ../include/vulkan
cp -R Vulkan-Headers/include/vulkan/ ../include/vulkan/

mkdir -p ../include/GLFW
cp -R glfw/include/GLFW/ ../include/GLFW/

mkdir -p ../include/stb
cp  stb/stb_image.h ../include/stb/

mkdir -p ../../engine/third-party/ImGui/
cp imgui/*.h ../../engine/third-party/ImGui/
cp imgui/*.cpp ../../engine/third-party/ImGui/

cp imgui/examples/imgui_impl_opengl3.h ../../engine/third-party/ImGui/
cp imgui/examples/imgui_impl_opengl3.cpp ../../engine/third-party/ImGui/

cp imgui/examples/imgui_impl_glfw.h ../../engine/third-party/ImGui/
cp imgui/examples/imgui_impl_glfw.cpp ../../engine/third-party/ImGui/

echo "#define IMGUI_IMPL_OPENGL_LOADER_GLAD" >temp.h.new
cat ../../engine/third-party/ImGui/imgui_impl_opengl3.h >>temp.h.new
mv -f temp.h.new ../../engine/third-party/ImGui/imgui_impl_opengl3.h

mkdir -p ../include/json/
cp json/single_include/nlohmann/json.hpp ../include/json/

cd ../

mkdir -p dll/linux
mkdir -p lib/linux
