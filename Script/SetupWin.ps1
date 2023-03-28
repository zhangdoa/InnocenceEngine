git submodule update

mkdir ../Source/External/GitSubmodules/assimp/build
mkdir ../Source/External/GitSubmodules/GLAD/build
mkdir ../Source/External/Include/GL
mkdir ../Source/External/Include/vulkan
mkdir ../Source/External/Include/DX12
mkdir ../Source/External/DLL/Win/Debug
mkdir ../Source/External/Lib/Win/Debug
mkdir ../Source/External/DLL/Win/Release
mkdir ../Source/External/Lib/Win/Release

mkdir ../Build
mkdir ../Bin

mkdir ../Res/ConvertedAssets
mkdir ../Res/Intermediate

Set-Location ../Source/External/GitSubmodules

Copy-Item -Recurse -Force Vulkan-Headers\include\* ..\Include\vulkan\
Copy-Item -Recurse -Force DirectX-Headers\include\* ..\Include\DX12\

Copy-Item -Recurse -Force assimp\include\assimp\* ..\Include\assimp\

Copy-Item -Force stb\stb_image.h ..\Include\stb\
Copy-Item -Force stb\stb_image_write.h ..\Include\stb\

Copy-Item -Force imgui\*.h ..\..\Engine\ThirdParty\ImGui\
Copy-Item -Force imgui\*.cpp ..\..\Engine\ThirdParty\ImGui\

Copy-Item -Force imgui\examples\imgui_impl_win32.h ..\..\Engine\ThirdParty\ImGui\
Copy-Item -Force imgui\examples\imgui_impl_win32.cpp ..\..\Engine\ThirdParty\ImGui\

Copy-Item -Force imgui\examples\imgui_impl_dx11.h ..\..\Engine\ThirdParty\ImGui\
Copy-Item -Force imgui\examples\imgui_impl_dx11.cpp ..\..\Engine\ThirdParty\ImGui\

Copy-Item -Force imgui\examples\imgui_impl_opengl3.h ..\..\Engine\ThirdParty\ImGui\
Copy-Item -Force imgui\examples\imgui_impl_opengl3.cpp ..\..\Engine\ThirdParty\ImGui\

Set-Content -Path ..\..\Engine\ThirdParty\ImGui\imgui_impl_opengl3.h -Value "#define IMGUI_IMPL_OPENGL_LOADER_GLAD`r`n$(Get-Content -Path ..\..\Engine\ThirdParty\ImGui\imgui_impl_opengl3.h)"

Copy-Item -Recurse -Force json\single_include\nlohmann\json.hpp ..\Include\json\

Copy-Item -Recurse -Force PhysX\physx\include\* ..\Include\PhysX\
Copy-Item -Recurse -Force PhysX\pxshared\include\* ..\Include\PhysX\

Copy-Item -Recurse -Force QDarkStyleSheet\qdarkstyle\rc\* ..\..\Editor\InnoEditor\qdarkstyle\rc\
Copy-Item -Force QDarkStyleSheet\qdarkstyle\style.qss ..\..\Editor\InnoEditor\qdarkstyle\
Copy-Item -Force QDarkStyleSheet\qdarkstyle\style.qrc ..\..\Editor\InnoEditor\qdarkstyle\

Invoke-WebRequest -Uri https://www.khronos.org/registry/OpenGL/api/GL/wglext.h -OutFile ..\Include\GL\wglext.h
Invoke-WebRequest -Uri https://www.khronos.org/registry/OpenGL/api/GL/glext.h -OutFile ..\Include\GL\glext.h

Set-Location ../
mkdir Tools
# Find the latest release
$(Invoke-WebRequest "https://github.com/microsoft/vswhere/releases/latest").BaseResponse.RequestMessage.RequestUri -match "tag/(.*)$"

# Download the vswhere.exe
Invoke-WebRequest "https://github.com/microsoft/vswhere/releases/download/$($Matches[1])/vswhere.exe" -OutFile "vswhere.exe"

Set-Location ../../Script