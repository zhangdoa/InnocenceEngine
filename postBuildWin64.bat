rd /s /q bin
mkdir bin
xcopy /s/e/y build\bin\Debug\* bin\
xcopy /s/e/y source\external\dll\win64\* bin\
xcopy /s/e/y source\external\dll\win64\* build\bin\Debug\
xcopy /s/e/y res\* build\engine\res\
pause