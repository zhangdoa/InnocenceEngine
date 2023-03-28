git submodule update

mkdir ../Source/External/GitSubmodules/assimp/build
mkdir ../Source/External/GitSubmodules/GLAD/build
mkdir ../Build
mkdir ../Bin

mkdir ../Res/ConvertedAssets
mkdir ../Res/Intermediate

Set-Location ../Source/External/GitSubmodules
Copy-Item -Recurse -Force assimp\include\assimp\* ..\Include\assimp\

Copy-Item -Force Vulkan-Headers\include\vulkan\* ..\Include\vulkan\

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

Copy-Item -Force json\single_include\nlohmann\json.hpp ..\Include\json\

Copy-Item -Recurse -Force PhysX\physx\include\* ..\Include\PhysX\
Copy-Item -Recurse -Force PhysX\pxshared\include\* ..\Include\PhysX\

Copy-Item -Recurse -Force QDarkStyleSheet\qdarkstyle\rc\* ..\..\Editor\InnoEditor\qdarkstyle\rc\
Copy-Item -Force QDarkStyleSheet\qdarkstyle\style.qss ..\..\Editor\InnoEditor\qdarkstyle\
Copy-Item -Force QDarkStyleSheet\qdarkstyle\style.qrc ..\..\Editor\InnoEditor\qdarkstyle\

Set-Location ../

mkdir Include\GL
Invoke-WebRequest -Uri https://www.khronos.org/registry/OpenGL/api/GL/wglext.h -OutFile Include\GL\wglext.h
Invoke-WebRequest -Uri https://www.khronos.org/registry/OpenGL/api/GL/glext.h -OutFile Include\GL\glext.h
mkdir Include\DX12
Invoke-WebRequest -Uri https://raw.githubusercontent.com/microsoft/DirectX-Headers/main/include/directx/d3dx12.h -OutFile Include\DX12\d3dx12.h

mkdir Tools
# Find the latest release
$(Invoke-WebRequest "https://github.com/microsoft/vswhere/releases/latest").BaseResponse.RequestMessage.RequestUri -match "tag/(.*)$"

# Download the vswhere.exe
Invoke-WebRequest "https://github.com/microsoft/vswhere/releases/download/$($Matches[1])/vswhere.exe" -OutFile "vswhere.exe"

mkdir DLL\Win\Debug
mkdir Lib\Win\Debug
mkdir DLL\Win\Release
mkdir Lib\Win\Release

Set-Location ../../Script