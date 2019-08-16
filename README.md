# Innocence Engine
![Screen capture](https://github.com/zhangdoa/InnocenceEngine/blob/master/ScreenCapture.jpg)
[![Trello website](https://img.shields.io/badge/Trello-Roadmap-00bfff.svg)](https://trello.com/b/iEYu58hu/innocence-engine)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/3c0ea60f7c46491d87236822f6de35a6)](https://www.codacy.com/app/zhangdoa/InnocenceEngine?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=zhangdoa/InnocenceEngine&amp;utm_campaign=Badge_Grade)
[![CodeFactor](https://www.codefactor.io/repository/github/zhangdoa/innocenceengine/badge)](https://www.codefactor.io/repository/github/zhangdoa/innocenceengine)
[![Build status](https://ci.appveyor.com/api/projects/status/hl31o0q6nbmlf83i?svg=true)](https://ci.appveyor.com/project/zhangdoa/innocenceengine)
[![GPL-3.0 licensed](https://img.shields.io/badge/license-GPL--3.0-brightgreen.svg)](LICENSE.md)
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine.svg?type=shield)](https://app.fossa.io/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine?ref=badge_shield)
[![Blog](https://img.shields.io/badge/My-Blog-ff884d.svg)](http://zhangdoa.com/)
[![Twitter Follow](https://img.shields.io/twitter/follow/espadrine.svg?style=social&label=Follow)](https://twitter.com/zhangdoa)
> "A poet once said, 'The whole universe is in a glass of wine.'"
> -- Richard Feynman, 1963

## Features

- Strict **E**ntity–**C**omponent–**S**ystem architecture, No OOP overhead/console-like programming experience/allow unlimited feature module extension.

```cpp
// the "E"
auto l_testEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Gameplay, "testEntity/");

// the "C"
auto l_testTransformComponent = SpawnComponent(TransformComponent, l_testEntity, ObjectSource::Runtime, ObjectUsage::Gameplay);

l_testTransformComponent->m_localTransformVector.m_pos = vec4(42.0f, 1.0f, PI<float>, 1.0f);

// the engine provided "S" will take care of other businesses
```

- Custom container, string and math classes, no STL overhead/No 3rd-party math library dependency.

```cpp
RingBuffer<float> l_testRingBuffer(32);
// All custom containers have a thread-safe version
Array<float, true> l_testThreadSafeArray;
FixedSizeString<64> l_testString;

l_testString = "Hello,World/";

for (size_t i = 0; i < l_testString.size(); i++)
{
	l_testRingBuffer.emplace_back((float)i);
}

l_testThreadSafeArray.reserve();
l_testThreadSafeArray.emplace_back(l_testRingBuffer[42]);

auto l_maxPoint = vec4(l_testThreadSafeArray[0], l_testRingBuffer[1], l_testRingBuffer[16], 1.0f);
auto l_minPoint = vec4(42.0f, l_testThreadSafeArray[0], -l_testRingBuffer[16], 1.0f);

auto l_testAABB = InnoMath::generateAABB(l_maxPoint, l_minPoint);
```

- Job-graph based parallel task model, fully utilize modern hardware/lock-free in client logic code.

```cpp
std::function<void()> f_JobA;
f_JobA = []()
{ 	
	g_pModuleManager->getLogSystem()->Log(LogLevel::Warning, "I'm worried that C++ would be quite a mess for me.");
};

auto f_JobB = [=](int val)
{ 
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "There are always some user-friendly programming languages waiting for you, just search for more than ", val, " seconds you'll find them."); 
};

// Job A will be executed on thread 5 as soon as possible
auto l_testTaskA = g_pModuleManager->getTaskSystem()->submit("ANonSenseTask", 5, nullptr, f_JobA);

// Job B will be executed with a parameter just after Job A finished
auto l_testTaskB = g_pModuleManager->getTaskSystem()->submit("NotANonSenseTask", 2, f_JobA, f_JobB, std::numeric_limits<int>::max());

// Blocking-wait on the caller thread
l_testTaskB->Wait();
```

- Object pool memory model, O(n) allocation/deallocation.

```cpp
struct POD
{
	float m_Float;
	int m_Int;
	void* m_Ptr;
}

auto l_objectPoolInstance =  g_pModuleManager->getMemorySystem()->createObjectPool(sizeof(POD), 65536);
auto l_PODInstance = l_objectPoolInstance->Spawn();
l_PODInstance->m_Float = 42.0f;
l_objectPoolInstance->Destroy(l_PODInstance);
```

- Logic Client as the plugin, the only coding rule is using the engine's interface to write your logic code and write whatever you want.

- The major graphics API support, from OpenGL 4.6 to DirectX 11, from DirectX 12 to Vulkan, and Metal, all supported by one unified interface.

- Client-Server rendering architecture, allow any kind of user-designed rendering pipeline from the first triangle draw call to the last swap chain presentation.

```cpp
auto l_renderingServer = g_pModuleManager->getRenderingServer();

// m_RPDC = l_renderingServer->AddRenderPassDataComponent("LightPass/");
l_renderingServer->CommandListBegin(m_RPDC, 0);
l_renderingServer->BindRenderPassDataComponent(m_RPDC);
l_renderingServer->CleanRenderTargets(m_RPDC);
l_renderingServer->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, m_SDC->m_ResourceBinder, 17, 0);
l_renderingServer->DispatchDrawCall(m_RPDC, m_quadMesh);
l_renderingServer->CommandListEnd(m_RPDC);

// Execute on separate threads
l_renderingServer->ExecuteCommandList(m_RPDC);
l_renderingServer->WaitForFrame(m_RPDC);
```

- Physically-based lighting, photometry lighting interface with support of real life light measurements like color temperature, luminous flux and so on.

- Default rendering client support features like:
  - Tiled-deferred rendering pipeline
  - Retro Blinn-Phong/Classic Cook-Torrance BRDF (Disney diffuse + Multi-scattering GGX specular) for opaque material
  - OIT rendering (w.r.t. transparency and thickness)
  - SSAO
  - CSM with VSM/PCF filters
  - Procedural sky
  - Large scale terrain rendering with cascaded tessellation
  - Motion Blur 
  - TAA
  - ACES tone mapping

- Real-time GI, baked PRT for the complex large scale scene or bake-free SVOGI for the limited small scale scene, fully dynamic and lightmap-free.

- Unified asset management, using popular JSON format for all text data and support easy-to-use binary data I/O.

- Physics simulation, NVIDIA's PhysX integrated.

- GUI Editor, Qt-based, easy to extend, easy to modify.

And so on...

## How to build?
All scripts are in /Script folder

### Windows

Tested OS version: Windows 10 version 1903

##### Prerequisites

- MSVC 19.00 or higher
- CMake 3.10 or higher 
- Vulkan pre-compiled library (Optional)
- Qt Creator 5.13 or higher

#### Build Engine

Run following scripts will build Debug and Release configurations in parallel:

``` cmd
@echo | SetupWin.bat
```
```powershell
BuildAssimpWin-VS15.ps1
BuildPhysXWin-VS15.ps1
BuildEngineWin-VS15.ps1
PostBuildWin.ps1
```

#### Build Editor

1. Open `Source\Editor\InnocenceEditor\InnocenceEditor.pro` with Qt Creator
2. Change "Projects - Build Settings - General - Build directory" to `..\..\..\Bin` for Debug, Profile and Release build configurations
3. Change "Projects - Run Settings - Run - Working directory" to `..\..\..\Bin`
4. Build the project

### Linux

Tested OS version: Ubuntu 18.04 LTS

##### Prerequisites

- GCC 8.0 or Clang 7.0 or higher 
- CMake 3.10 or higher 
- OpenGL library(lGL)

#### Build Engine

Run following scripts:

``` shell
echo | SetupLinux.sh
echo | BuildAssimpLinux.sh
echo | BuildLinux.sh # or BuildLinux-Clang.sh or BuildLinux-CodeBlocks.sh
echo | PostBuildLinux.sh
```

### macOS

Tested OS version : macOS 10.13.6

##### Prerequisites

- CMake 3.10 or higher 
- Apple Clang 10.0 or LLVM Clang 8.0 or higher

#### Build Engine

Run following scripts:

``` shell
echo | SetupMac.sh
echo | BuildAssimpMac-Xcode.sh
echo | BuildMac.sh
echo | PostBuildMac.sh
```

## How to use?

### Windows

#### Launch game

Run following script:

``` powershell
StartEngineWin.ps1
```

#### Launch editor

Launch through Qt Creator

### Linux

Run following script:

``` shell
echo | StartEngineLinux.sh
```

### macOS

Run following script:

``` shell
echo | StartEngineMac.sh
```

## How to debug

### Windows

1. Open Build/InnocenceEngine.sln
2. Set debug launch arguments and default launch project to InnoMain
3. Start debug with "Launch" button (default F5)

### Linux

1. Use Atom to load the working copy folder
2. Install gcc-compiler package
3. Select build/makefile and hit "Compile and Debug" button (default F6)
4. (Optional) Change launch arguments in Source/Engine/Platform/LinuxMain/CMakeLists.txt

### macOS

1. Use Atom to load the working copy folder
2. Open Source/Engine/Platform/MacOS/InnoMain.InnoMain.xcodeproj
3. Select "Product" - "Run" (⌘ + R)

## How to bake scene?

### Windows

Run following script:

``` powershell
BakeScene.ps1 -sceneName [scene file name without extension]
```

## Available launch arguments

```
-mode [value]
```
| Value |Notes |
| --- | --- |
| 0 | engine will handle the window creation and event management, for "slave-client" model like the normal game client |
| 1 | engine requires client providing an external window handle, for "host-client" model like the external editor |

```
-renderer [value]
```
| Value | --- | Notes |
| --- | --- | --- |
| 0 | OpenGL | Not available on macOS, currently supported version 4.6 |
| 1 | DirectX 11 |  Only available on Windows, currently supported version 11.4 |
| 2 | DirectX 12 | Only available on Windows, currently supported version 12.1 |
| 3 | Vulkan | Not available on macOS, currently supported 1.1.92.1 (WIP) |
| 4 | Metal | Only available on macOS, currently supported version 2 (WIP) |

```
-loglevel [value]
```
| Value |Notes |
| --- | --- |
| 0 | print verbose level and all the other higher level log messages |
| 1 | only print success level and higher level log messages |
| 2 | only print warning level and higher level log messages |
| 3 | only print error level log messages |

## Shader languages compatibilities

- OpenGL rendering server requires SPIR-V binary format shader file for release build, please use `ParseGLShader.bat` and `CompileGLShaderToSPIR-V.bat` to generate them.

- Vulkan rendering server requires SPIR-V binary format shader file,  please use `CompileVKShaderToSPIR-V.bat` to generate them.

- Use `ConvertSPIR-VToGLSL.bat` and `ConvertSPIR-VToHLSL.bat` to generate human readable GLSL and HLSL files from SPIR-V binary shader file. 

## License
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine.svg?type=large)](https://app.fossa.io/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine?ref=badge_large)

## References & Dependencies

### Third-party libraries

[assimp](https://github.com/assimp)

[GLAD](https://github.com/Dav1dde/glad)

[dear imgui](https://github.com/ocornut/imgui)

[stb](https://github.com/nothings/stb)

[JSON for Modern C++](https://github.com/nlohmann/json)

[PhysX](https://github.com/NVIDIAGameWorks/PhysX)

### Assets

[Free3D]( https://thefree3dmodels.com)

[Musopen](https://musopen.org)

[Free PBR Materials](https://freepbr.com/)

[HDR Labs](http://www.hdrlabs.com/)

## Inspirations

### Books

[C++ Primer (4th Edition)](https://www.amazon.com/Primer-4th-Stanley-B-Lippman/dp/0201721481)

[A Tour of C++](https://www.amazon.com/Tour-C-Depth/dp/0321958314)

[Effective C++: 55 Specific Ways to Improve Your Programs and Designs (3rd Edition)](https://www.amazon.com/Effective-Specific-Improve-Programs-Designs/dp/0321334876)

[Inside the C++ Object Model (1st Edition)](https://www.amazon.com/Inside-Object-Model-Stanley-Lippman/dp/0201834545)

[Effective Modern C++: 42 Specific Ways to Improve Your Use of C++11 and C++14](https://www.amazon.com/Effective-Modern-Specific-Ways-Improve/dp/1491903996)

[API Design for C++ (1st Edition)](https://www.amazon.com/API-Design-C-Martin-Reddy/dp/0123850037)

[Advanced C and C++ Compiling (1st Edition)](https://www.amazon.com/Advanced-C-Compiling-Milan-Stevanovic/dp/1430266678)

[Data Structures and Algorithms in C++ (4th Edition)](https://www.amazon.com/Data-Structures-Algorithms-Adam-Drozdek-ebook/dp/B00B6F0F5S)

[Game Engine Architecture (1st Edition)](https://www.amazon.com/Game-Engine-Architecture-Jason-Gregory/dp/1568814135)

[Game Programming Patterns](https://www.amazon.com/Game-Programming-Patterns-Robert-Nystrom/dp/0990582906)

[Game Coding Complete (4th Edition)](https://www.amazon.com/Game-Coding-Complete-Fourth-McShaffry/dp/1133776574)

[Real-Time Rendering (4th Edition)](https://www.amazon.com/Real-Time-Rendering-Fourth-Tomas-Akenine-Mo-ebook/dp/B07FSKB982)

[Physically Based Rendering : From Theory to Implementation(2nd Edition)](https://www.amazon.com/Physically-Based-Rendering-Second-Implementation/dp/0123750792)

[Computer Graphics with Open GL (4th Edition)](https://www.amazon.com/Computer-Graphics-Open-GL-4th/dp/0136053580)

[OpenGL Programming Guide: The Official Guide to Learning OpenGL, Version 4.5 with SPIR-V (9th Edition)](https://www.amazon.com/OpenGL-Programming-Guide-Official-Learning/dp/0134495497)

[Calculus (6th Edition)](https://www.amazon.com/CALCULUS-Sixth-James-Stewart/dp/B00722RNC2)

[Linear Algebra and Its Applications (3rd Edition)](https://www.amazon.com/Linear-Algebra-Its-Applications-3rd/dp/0201709708)

And more...

### Online tutorials & resources

[cppreference.com](https://en.cppreference.com)

[Standard C++](https://isocpp.org)

[Modernes C++](http://modernescpp.com/index.php)

[Mathematics - Martin Baker](http://www.euclideanspace.com/maths)

[Wolfram MathWorld: The Web's Most Extensive Mathematics Resource](http://mathworld.wolfram.com)

[GameDev.net](https://www.gamedev.net/)

[Gamasutra](http://www.gamasutra.com)

[Scratchapixel](https://www.scratchapixel.com)

[Advances in Real-Time Rendering in 3D Graphics and Games](http://advances.realtimerendering.com)

[OpenGL Wiki](https://www.khronos.org/opengl/wiki)

[Learn OpenGL](https://learnopengl.com)

[OpenGL Step by Step](http://ogldev.atspace.co.uk)

[DirectX 11 official documents](https://docs.microsoft.com/en-us/windows/win32/direct3d11/atoc-dx-graphics-direct3d-11)

[DirectX 12 official documents](https://docs.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-graphics)

[RasterTek - DirectX 10, DirectX 11, and DirectX 12 tutorials](http://www.rastertek.com/)

[Vulkan official documents](https://www.khronos.org/registry/vulkan/)

[Vulkan Tutorial](https://vulkan-tutorial.com/)

[Metal official documents](https://developer.apple.com/metal/)

[Sébastien Lagarde's blog](https://seblagarde.wordpress.com)

[Stephen Hill's blog](http://blog.selfshadow.com)

[thebennybox's YouTube channel](https://www.youtube.com/user/thebennybox)

[Randy Gaul's Game Programming Blog](http://www.randygaul.net)

And more...
