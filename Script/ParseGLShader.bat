mkdir ..\Res\Shaders\Parsed
cd ../Res/Shaders/Parsed
del /S /Q *.*

cd ../GL
for %%i in (*) do start ../../../Bin/Debug/InnoGLSLParser.exe %%i ../Parsed/%%~i
pause