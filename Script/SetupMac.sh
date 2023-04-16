#!/bin/sh
#git submodule update

cd ../Source/External/GitSubmodules

mkdir -p ../../Engine/ThirdParty/ImGui/
cp imgui/*.h ../../Engine/ThirdParty/ImGui/
cp imgui/*.cpp ../../Engine/ThirdParty/ImGui/

cp imgui/examples/imgui_impl_opengl3.h ../../Engine/ThirdParty/ImGui/
cp imgui/examples/imgui_impl_opengl3.cpp ../../Engine/ThirdParty/ImGui/

echo "#define IMGUI_IMPL_OPENGL_LOADER_GLAD" >temp.h.new
cat ../../Engine/ThirdParty/ImGui/imgui_impl_opengl3.h >>temp.h.new
mv -f temp.h.new ../../Engine/ThirdParty/ImGui/imgui_impl_opengl3.h

mkdir -p ../../editor/InnoEditor/qdarkstyle/
mkdir -p ../../editor/InnoEditor/qdarkstyle/rc/
cp QDarkStyleSheet/qdarkstyle/rc/* ../../Editor/InnoEditor/qdarkstyle/rc/
cp QDarkStyleSheet/qdarkstyle/style.qss ../../Editor/InnoEditor/qdarkstyle/
cp QDarkStyleSheet/qdarkstyle/style.qrc ../../Editor/InnoEditor/qdarkstyle/

cd ../

mkdir -p DLL/Mac/Debug
mkdir -p Lib/Mac/Debug
mkdir -p DLL/Mac/Release
mkdir -p Lib/Mac/Release
