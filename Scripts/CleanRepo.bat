@echo off

cd ../
echo "Removing local generated files..."
rd /s /q Bin
rd /s /q Build
rd /s /q Source\Editor\Build
del /s /q Source\Engine\Common\Config.h

echo "Done!"