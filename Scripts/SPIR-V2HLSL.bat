mkdir ..\Res\Shaders\Converted
cd ../Res/Shaders/Converted
del /S /Q *.*

cd ../SPIRV
for %%i in (*) do spirv-cross.exe %%i --hlsl --output ../Converted/%%~i
pause