rd /s /q bin
mkdir bin

REM copy binary
xcopy /s/e/y build\bin\Debug\* bin\

REM copy assets
xcopy /s/e/y res\* build\bin\res\
xcopy /s/e/y res\* bin\res\

REM copy dll
xcopy /s/e/y source\external\dll\win\* build\bin\Debug\
xcopy /s/e/y source\external\dll\win\* bin\

pause