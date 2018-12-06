git submodule update

cd source/external/gitsubmodules
xcopy /s/e/y assimp\include\assimp\* ..\include\assimp\
xcopy /s/e/y assimp\build_dll\include\assimp\config.h ..\include\assimp\

xcopy /s/e/y Vulkan-Headers\include\vulkan\* ..\include\vulkan\

xcopy /s/e/y glfw\include\GLFW\* ..\include\GLFW\

xcopy /s/e/y stb\stb_image.h ..\include\stb\

cd ../

mkdir dll\win64
mkdir lib\win64

pause