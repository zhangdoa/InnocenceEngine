mkdir ..\Res\Shaders\SPIRV
cd ../Res/Shaders/SPIRV
del /S /Q *.spv

cd ../HLSL
for %%i in (*) do glslangValidator.exe -V -t -e main -D -o ../SPIRV/%%~i.spv %%i
pause