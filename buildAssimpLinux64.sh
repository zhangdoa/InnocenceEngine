cd source/external/gitsubmodules/assimp

mkdir build_dll
cd build_dll
cmake -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF -G "Unix Makefiles" ../
make

#cp code\Debug\*.dll ..\..\..\dll\win64
#cp /s/e/y code\Debug\assimp.lib ..\..\..\lib\win64

#cd ../
#xcopy /s/e/y include\assimp\* ..\..\include\assimp\
#xcopy /s/e/y build_dll\include\assimp\config.h ..\..\include\assimp\
