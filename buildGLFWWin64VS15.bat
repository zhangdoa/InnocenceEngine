cd source/external/gitsubmodules/glfw

mkdir build_dll
cd build_dll
cmake -DBUILD_SHARED_LIBS=ON -G "Visual Studio 15 Win64" ../
msbuild GLFW.sln

xcopy /s/e/y src\Debug\*.dll ..\..\..\dll\win64
xcopy /s/e/y src\Debug\*.lib ..\..\..\lib\win64

pause

