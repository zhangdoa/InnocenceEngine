cd ../Res/Shaders/Converted
del /S /Q *.*

cd ../SPIRV
for %%i in (*) do spirv-cross.exe %%i --output ../Converted/%%~ni
pause