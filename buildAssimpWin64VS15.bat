cd source/external/gitsubmodules/assimp

mkdir build_dll
cd build_dll
cmake -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF -G "Visual Studio 15 Win64" ../
msbuild Assimp.sln

xcopy /s/e/y code\Debug\*.dll ..\..\..\dll\win64
rename code\Debug\assimp-vc140-mt.lib assimp.lib
xcopy /s/e/y code\Debug\assimp.lib ..\..\..\lib\win64
pause

