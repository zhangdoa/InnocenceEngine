mkdir build
cd build
cmake -G "Unix Makefiles" ../source
make

cd ../
rd /s /q bin
mkdir bin
xcopy /s/e/y build\bin\Debug\* bin\
xcopy /s/e/y source\external\dll\* bin\
xcopy /s/e/y source\external\dll\* build\bin\Debug\
xcopy /s/e/y res\* build\engine\res\
pause