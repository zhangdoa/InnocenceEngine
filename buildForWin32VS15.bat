mkdir build
cd build
cmake -G "Visual Studio 15" ../source
msbuild InnocenceEngine.sln

cd ../
rd /s /q bin
mkdir bin
xcopy /s/e/y build\bin\Debug\* bin\
xcopy /s/e/y source\external\dll\win32\* bin\
xcopy /s/e/y source\external\dll\win32\* build\bin\Debug\
xcopy /s/e/y res\* build\engine\res\
pause