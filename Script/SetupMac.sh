#!/bin/sh
#git submodule update

cd ../Source/External/GitSubmodules

mkdir -p ../Include/assimp
cp -R assimp/include/assimp/ ../Include/assimp/

mkdir -p ../Include/vulkan
cp -R Vulkan-Headers/include/vulkan/ ../Include/vulkan/

mkdir -p ../Include/stb
cp stb/stb_image.h ../Include/stb/
cp stb/stb_image_write.h ../Include/stb/

mkdir -p ../../Engine/ThirdParty/ImGui/
cp imgui/*.h ../../Engine/ThirdParty/ImGui/
cp imgui/*.cpp ../../Engine/ThirdParty/ImGui/

cp imgui/examples/imgui_impl_opengl3.h ../../Engine/ThirdParty/ImGui/
cp imgui/examples/imgui_impl_opengl3.cpp ../../Engine/ThirdParty/ImGui/

echo "#define IMGUI_IMPL_OPENGL_LOADER_GLAD" >temp.h.new
cat ../../Engine/ThirdParty/ImGui/imgui_impl_opengl3.h >>temp.h.new
mv -f temp.h.new ../../Engine/ThirdParty/ImGui/imgui_impl_opengl3.h

mkdir -p ../Include/json/
cp json/single_include/nlohmann/json.hpp ../Include/json/

mkdir -p ../../editor/InnocenceEditor/darkstyle/
cp Qt-Frameless-Window-DarkStyle/darkstyle/* ../../Editor/InnocenceEditor/darkstyle/
cp Qt-Frameless-Window-DarkStyle/darkstyle.qrc ../../Editor/InnocenceEditor/
cp Qt-Frameless-Window-DarkStyle/DarkStyle.h ../../Editor/InnocenceEditor/
cp Qt-Frameless-Window-DarkStyle/DarkStyle.cpp ../../Editor/InnocenceEditor/

cd ../

mkdir -p DLL/Mac
mkdir -p Lib/Mac
