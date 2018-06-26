mkdir build
cd build
cmake -DINNO_PLATFORM_WIN32=OFF -DINNO_PLATFORM_WIN64=ON -DINNO_PLATFORM_LINUX64=OFF -G "Visual Studio 15 Win64" ../source
cmake -DINNO_PLATFORM_WIN32=OFF -DINNO_PLATFORM_WIN64=ON -DINNO_PLATFORM_LINUX64=OFF -G "Visual Studio 15 Win64" ../source
msbuild InnocenceEngine.sln