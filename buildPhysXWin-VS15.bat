cd source/external/gitsubmodules/PhysX/physx

@echo | generate_projects.bat vc15win64

msbuild compiler/vc15win64/PhysXSDK.sln

xcopy /s/e/y bin\win.x86_64.vc141.mt\debug\*.dll ..\..\..\dll\win
xcopy /s/e/y bin\win.x86_64.vc141.mt\debug\*.lib ..\..\..\lib\win

pause