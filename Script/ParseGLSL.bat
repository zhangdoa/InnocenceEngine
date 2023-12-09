mkdir ..\Res\Shaders\Parsed
cd ../Res/Shaders/Parsed
del /S /Q *.*

cd ../GLSL
for %%i in (*) do start ../../../Bin/Debug/GLSLParser.exe %%i ../Parsed/%%~i
pause