cd source/external/gitsubmodules/assimp

mkdir build_lib
cd build_lib
cmake -DBUILD_SHARED_LIBS=OFF -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF -G "Visual Studio 15 Win64" ../
msbuild Assimp.sln
rename code\Debug\assimp-vc140-mt.lib assimp.lib
xcopy /s/e/y code\Debug\assimp.lib ..\..\..\lib\win64

cd ../
mkdir build_dll
cd build_dll
cmake -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF -G "Visual Studio 15 Win64" ../
msbuild Assimp.sln
rename code\Debug\assimp-vc140-mt.dll assimp.dll
xcopy /s/e/y code\Debug\assimp.dll ..\..\..\dll\win64

cd ../
xcopy /s/e/y include\assimp\* ..\..\include\assimp\
xcopy /s/e/y build_dll\include\assimp\config.h ..\..\include\assimp\
pause

