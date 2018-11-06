mkdir build
cd build
cmake -DINNO_PLATFORM_WIN64=ON -DBUILD_GAME=ON -G "Visual Studio 15 Win64" ../source
cmake -DINNO_PLATFORM_WIN64=ON -DBUILD_GAME=ON -G "Visual Studio 15 Win64" ../source
msbuild InnocenceEngine.sln
pause