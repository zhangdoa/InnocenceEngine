mkdir ..\Res\Shaders\SPIRV
cd ../Res/Shaders/SPIRV
del /S /Q *.spv

cd ../Parsed
for %%i in (*) do glslangValidator.exe -V -t -o ../SPIRV/%%~i.spv %%i
pause