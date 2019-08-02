cd ../Res/Shaders/DX11
del /S /Q *.spv
for %%i in (*) do glslangValidator.exe -G -e main -D -o ../SPIRV/%%~i.spv %%i
pause