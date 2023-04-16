git submodule update

mkdir ../Source/External/GitSubmodules/assimp/build
mkdir ../Source/External/GitSubmodules/GLAD/build
mkdir ../Source/External/Include/GL
mkdir ../Source/External/DLL/Win/Debug
mkdir ../Source/External/Lib/Win/Debug
mkdir ../Source/External/DLL/Win/Release
mkdir ../Source/External/Lib/Win/Release

mkdir ../Build
mkdir ../Bin

mkdir ../Res/ConvertedAssets
mkdir ../Res/Intermediate

Set-Location ../Source/External/GitSubmodules

Copy-Item -Force imgui\*.h ..\..\Engine\ThirdParty\ImGui\
Copy-Item -Force imgui\*.cpp ..\..\Engine\ThirdParty\ImGui\

Copy-Item -Force imgui\backends\imgui_impl_win32.h ..\..\Engine\ThirdParty\ImGui\
Copy-Item -Force imgui\backends\imgui_impl_win32.cpp ..\..\Engine\ThirdParty\ImGui\

Copy-Item -Force imgui\backends\imgui_impl_dx11.h ..\..\Engine\ThirdParty\ImGui\
Copy-Item -Force imgui\backends\imgui_impl_dx11.cpp ..\..\Engine\ThirdParty\ImGui\

Copy-Item -Force imgui\backends\imgui_impl_opengl3.h ..\..\Engine\ThirdParty\ImGui\
Copy-Item -Force imgui\backends\imgui_impl_opengl3.cpp ..\..\Engine\ThirdParty\ImGui\
Copy-Item -Force imgui\backends\imgui_impl_opengl3_loader.h ..\..\Engine\ThirdParty\ImGui\

Copy-Item -Recurse -Force QDarkStyleSheet\qdarkstyle\rc\* ..\..\Editor\InnoEditor\qdarkstyle\rc\
Copy-Item -Force QDarkStyleSheet\qdarkstyle\style.qss ..\..\Editor\InnoEditor\qdarkstyle\
Copy-Item -Force QDarkStyleSheet\qdarkstyle\style.qrc ..\..\Editor\InnoEditor\qdarkstyle\

Invoke-WebRequest -Uri https://www.khronos.org/registry/OpenGL/api/GL/wglext.h -OutFile ..\Include\GL\wglext.h
Invoke-WebRequest -Uri https://www.khronos.org/registry/OpenGL/api/GL/glext.h -OutFile ..\Include\GL\glext.h

Set-Location ../
mkdir Tools
Set-Location Tools

$headers = @{
    "Accept" = "application/vnd.github.v3+json"
    "User-Agent" = "PowerShell"
}

# Find the latest release version number
$url = "https://api.github.com/repos/microsoft/vswhere/releases/latest"
$response = Invoke-RestMethod -Uri $url -Headers $headers
$version = $response.tag_name

# Download the vswhere.exe
$downloadUrl = "https://github.com/microsoft/vswhere/releases/download/$version/vswhere.exe"
Invoke-WebRequest $downloadUrl -OutFile "vswhere.exe"

# Find the latest release version
$url = "https://api.github.com/repos/microsoft/DirectXShaderCompiler/releases/latest"
$response = Invoke-RestMethod -Uri $url -Headers $headers
$version = $response.tag_name

# Find the asset download URL
foreach ($asset in $response.assets) {
    if ($asset.name -like "dxc_*") {
        $downloadUrl = $asset.browser_download_url
        break
    }
}

# Download the DXIL compiler binaries
Invoke-WebRequest $downloadUrl -OutFile "dxc.zip"

# Extract the contents of the archive
Expand-Archive "dxc.zip"
Copy-Item -Recurse -Force dxc\bin\x64\* .

Set-Location ../../../Script