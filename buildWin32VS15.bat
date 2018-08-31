mkdir build
cd build
cmake -DINNO_PLATFORM_WIN32=ON -DINNO_RENDERER_OPENGL=ON -DBUILD_GAME=ON -G "Visual Studio 15" ../source
cmake -DINNO_PLATFORM_WIN32=ON -DINNO_RENDERER_OPENGL=ON -DBUILD_GAME=ON -G "Visual Studio 15" ../source
msbuild InnocenceEngine.sln
pause