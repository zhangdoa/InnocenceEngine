# Innocence Engine
![Screen capture](https://github.com/zhangdoa/InnocenceEngine/blob/master/ScreenCapture.jpg)
[![Trello website](https://img.shields.io/badge/Trello-Roadmap-00bfff.svg)](https://trello.com/b/iEYu58hu/innocence-engine)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/3c0ea60f7c46491d87236822f6de35a6)](https://www.codacy.com/app/zhangdoa/InnocenceEngine?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=zhangdoa/InnocenceEngine&amp;utm_campaign=Badge_Grade)
[![CodeFactor](https://www.codefactor.io/repository/github/zhangdoa/innocenceengine/badge)](https://www.codefactor.io/repository/github/zhangdoa/innocenceengine)
[![Build status](https://ci.appveyor.com/api/projects/status/hl31o0q6nbmlf83i?svg=true)](https://ci.appveyor.com/project/zhangdoa/innocenceengine)
[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine.svg?type=shield)](https://app.fossa.io/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine?ref=badge_shield)
[![Blog](https://img.shields.io/badge/My-Blog-ff884d.svg)](http://zhangdoa.com/)
[![Twitter Follow](https://img.shields.io/twitter/follow/espadrine.svg?style=social&label=Follow)](https://twitter.com/zhangdoa)
> "A poet once said, 'The whole universe is in a glass of wine.'"
> -- Richard Feynman, 1963

## License
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine.svg?type=large)](https://app.fossa.io/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine?ref=badge_large)

## How to build

Currently avaliable platforms:

### Windows:

Tested under Windows 10 version 1809

Prerequisites: MSVC 19.00 + CMake 3.10 or higher

Run following scripts in a sequence:

``` cmd
@echo | setupWin.bat
@echo | buildAssimpWin64VS15.bat
@echo | buildGLFWWin64VS15.bat
@echo | buildWin64VS15.bat
@echo | postBuildWin64.bat
```

### Linux:

Tested under Ubuntu 18.04 LTS

Prerequisites: GCC 7.0 or Clang 6.0 or higher + CMake 3.10 or higher + OpenGL library(lGL)

Run following scripts in a sequence:

``` shell
echo | setupLinux.sh
echo | buildAssimpLinux64.sh
echo | buildGLFWLinux64.sh
echo | buildLinux64.sh # or buildLinux64-Clang.sh or buildLinux64-CodeBlocks.sh
echo | postBuildLinux64.sh
```

### Mac OSX:

Tested under version 10.13.6

Prerequisites: CMake 3.10 or higher + XCode 9.4.1 or higher

Run following scripts in a sequence:

``` shell
echo | setupMac.sh
echo | buildAssimpMac64-Xcode.sh
echo | buildGLFWMac64-Xcode.sh
echo | buildMac64-Xcode.sh
echo | postBuildMac64.sh
```

## Features

### Architecture

[Entity–component–system](https://en.wikipedia.org/wiki/Entity%E2%80%93component%E2%80%93system)

### Rendering

[Deferred shading](https://en.wikipedia.org/wiki/Deferred_shading)

[Physically based rendering](https://en.wikipedia.org/wiki/Physically_based_rendering)

[Scene graph](https://en.wikipedia.org/wiki/Scene_graph)

## References& Dependencies

### Third-party libraries

[assimp](https://github.com/assimp)

[GLFW](https://github.com/glfw/glfw)

[GLAD](https://github.com/Dav1dde/glad) 

[dear imgui](https://github.com/ocornut/imgui)

[stb](https://github.com/nothings/stb)

[JSON for Modern C++](https://github.com/nlohmann/json)

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

### Online tutorials& resources

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
