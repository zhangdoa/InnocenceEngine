mkdir build
cd build
cmake -DINNO_PLATFORM_WIN32=ON -DBUILD_GAME=ON -G "Visual Studio 15" ../source
cmake -DINNO_PLATFORM_WIN32=ON -DBUILD_GAME=ON -G "Visual Studio 15" ../source
msbuild InnocenceEngine.sln
pause