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

- Strict Entity–Component–System architecture

No OOP overhead/Console-like programming experience/Allow unlimited feature module extension

- Custom container, string and math classes

No STL overhead/No 3rd-party math library dependency

- Job-graph based parallel task model

Fully utilize modern hardware / lock-free in client logic code

- Object pool memory model

O(n) allocation/deallocation

- Logic Client as the plugin

The only rule is using the engine's interface to write your logic code, and write whatever you want

- The major graphics API support

From OpenGL 4.6 to DirectX 11, from DirectX 12 to Vulkan, and Metal, all supported by one unified interface

- Client-Server rendering architecture

Allow fully user-designed rendering pipeline from the first triangle to the last swap chain

- Physically-based lighting

Photometry lighting interface

- Default rendering client support features like
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

- Real-time GI

Baked PRT for the complex large scale scene or bake-free SVOGI for the limited small scale scene, fully dynamic and lightmap-free

- Unified asset management

Using popular JSON format for all text data

- Physics simulation

NVIDIA's PhysX integrated

- GUI Editor

Qt-based, easy to extend and no coupling with the engine

And so on...

## License
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine.svg?type=large)](https://app.fossa.io/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine?ref=badge_large)

## How to build
All scripts are in /Script folder

### Windows

#### Build Engine
Tested OS version: Windows 10 version 1903

##### Prerequisites
- MSVC 19.00 or higher
- CMake 3.10 or higher 
- Vulkan pre-compiled library (Optional)

Run following scripts will build Debug and Release configurations in parallel

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
Tested OS version: Windows 10 1903

##### Prerequisites
- Qt Creator 5.13 or higher

1. Open `Source\Editor\InnocenceEditor\InnocenceEditor.pro` with Qt Creator
2. Change "Projects - Build Settings - General - Build directory" to `..\..\..\Bin` for Debug, Profile and Release build configurations
3. Change "Projects - Run Settings - Run - Working directory" to `..\..\..\Bin`
4. Build the project

### Linux
Tested OS version: Ubuntu 18.04 LTS

#### Build Engine

##### Prerequisites
- GCC 8.0 or Clang 7.0 or higher 
- CMake 3.10 or higher 
- OpenGL library(lGL)

Run following scripts

``` shell
echo | SetupLinux.sh
echo | BuildAssimpLinux.sh
echo | BuildLinux.sh # or BuildLinux-Clang.sh or BuildLinux-CodeBlocks.sh
echo | PostBuildLinux.sh
```

### macOS
Tested OS version: macOS 10.13.6

#### Build Engine

##### Prerequisites
- CMake 3.10 or higher 
- Apple Clang 10.0 or LLVM Clang 8.0 or higher

Run following scripts

``` shell
echo | SetupMac.sh
echo | BuildAssimpMac-Xcode.sh
echo | BuildMac.sh
echo | PostBuildMac.sh
```

## How to use

### Windows

#### Launch game
Run following script

``` powershell
StartEngineWin.ps1
```

#### Launch editor
Launch through Qt Creator

### Linux
Run following script

``` shell
echo | StartEngineLinux.sh
```

### macOS
Run following script

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

## Available launch arguments

```
-renderer [value]
```
| Value | --- | Notes |
| --- | --- | --- |
| 0 | OpenGL | Not available on macOS, current support version 4.6 |
| 1 | DirectX 11 |  Only available on Windows, current support version 11.4 |
| 2 | DirectX 12 | Only available on Windows, current support version 12.1 |
| 3 | Vulkan | Not available on macOS, current support version 1.1.92.1 |
| 4 | Metal | Only available on macOS, current support version 2 |

```
-mode [value]
```
| Value |Notes |
| --- | --- |
| 0 | game mode |
| 1 | reserved for editor |

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

[Mathematics - Martin Baker](http://www.euclideanspace.com/maths)

[Wolfram MathWorld: The Web's Most Extensive Mathematics Resource](http://mathworld.wolfram.com)

[cppreference.com](https://en.cppreference.com)

[Standard C++](https://isocpp.org)

[Gamasutra](http://www.gamasutra.com)

[Scratchapixel](https://www.scratchapixel.com)

[Advances in Real-Time Rendering in 3D Graphics and Games](http://advances.realtimerendering.com)

[Learn OpenGL](https://learnopengl.com)

[OpenGL Step by Step](http://ogldev.atspace.co.uk)

[RasterTek - DirectX 10, DirectX 11, and DirectX 12 tutorials](http://www.rastertek.com/)

[thebennybox's YouTube channel](https://www.youtube.com/user/thebennybox)

[Randy Gaul's Game Programming Blog](http://www.randygaul.net)

[Sébastien Lagarde's blog](https://seblagarde.wordpress.com)

[Stephen Hill's blog](http://blog.selfshadow.com)

And more...
