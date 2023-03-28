git submodule update

mkdir ..\Res\ConvertedAssets
mkdir ..\Res\Intermediate

cd ../Source/External/GitSubmodules
xcopy /e/s/y assimp\include\assimp\* ..\Include\assimp\

xcopy /y Vulkan-Headers\include\vulkan\* ..\Include\vulkan\

xcopy /y stb\stb_image.h ..\Include\stb\
xcopy /y stb\stb_image_write.h ..\Include\stb\

xcopy /y imgui\*.h ..\..\Engine\ThirdParty\ImGui\
xcopy /y imgui\*.cpp ..\..\Engine\ThirdParty\ImGui\

xcopy /y imgui\examples\imgui_impl_win32.h ..\..\Engine\ThirdParty\ImGui\
xcopy /y imgui\examples\imgui_impl_win32.cpp ..\..\Engine\ThirdParty\ImGui\

xcopy /y imgui\examples\imgui_impl_dx11.h ..\..\Engine\ThirdParty\ImGui\
xcopy /y imgui\examples\imgui_impl_dx11.cpp ..\..\Engine\ThirdParty\ImGui\

xcopy /y imgui\examples\imgui_impl_opengl3.h ..\..\Engine\ThirdParty\ImGui\
xcopy /y imgui\examples\imgui_impl_opengl3.cpp ..\..\Engine\ThirdParty\ImGui\

(echo #define IMGUI_IMPL_OPENGL_LOADER_GLAD) >temp.h.new
type ..\..\Engine\ThirdParty\ImGui\imgui_impl_opengl3.h >>temp.h.new
move /y temp.h.new ..\..\Engine\ThirdParty\ImGui\imgui_impl_opengl3.h

xcopy /y json\single_include\nlohmann\json.hpp ..\Include\json\

xcopy /e/s/y PhysX\physx\include\* ..\Include\PhysX\
xcopy /e/s/y PhysX\pxshared\include\* ..\Include\PhysX\

xcopy /e/s/y QDarkStyleSheet\qdarkstyle\rc\* ..\..\Editor\InnoEditor\qdarkstyle\rc\
xcopy /y QDarkStyleSheet\qdarkstyle\style.qss ..\..\Editor\InnoEditor\qdarkstyle\
xcopy /y QDarkStyleSheet\qdarkstyle\style.qrc ..\..\Editor\InnoEditor\qdarkstyle\

cd ../

mkdir Include\GL
powershell -Command "Invoke-WebRequest https://www.khronos.org/registry/OpenGL/api/GL/wglext.h -OutFile Include\GL\wglext.h"
powershell -Command "Invoke-WebRequest https://www.khronos.org/registry/OpenGL/api/GL/glext.h -OutFile Include\GL\glext.h"
mkdir Include\DX12
powershell -Command "Invoke-WebRequest https://raw.githubusercontent.com/microsoft/DirectX-Headers/main/include/directx/d3dx12.h -OutFile Include\DX12\d3dx12.h"

mkdir Tools
powershell -Command "Invoke-WebRequest https://github.com/microsoft/vswhere/releases/download/3.0.3/vswhere.exe -OutFile Tools\vswhere.exe"

mkdir DLL\Win\Debug
mkdir Lib\Win\Debug
mkdir DLL\Win\Release
mkdir Lib\Win\Release

cd ../../Script