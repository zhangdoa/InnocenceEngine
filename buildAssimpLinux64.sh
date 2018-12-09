cd source/external/gitsubmodules/assimp

mkdir build_dll
cd build_dll
cmake -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_TESTS=OFF -G "Unix Makefiles" ../
make

cp include/assimp/config.h ../../../include/assimp/
cp code/Debug/*.so ../../../lib/unix