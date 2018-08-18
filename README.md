# Innocence Engine
![Screen capture](https://github.com/zhangdoa/InnocenceEngine/blob/master/Capture_processed.jpg)
[![Trello website](https://img.shields.io/badge/Trello-Roadmap-00bfff.svg)](https://trello.com/b/iEYu58hu/innocence-engine)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/3c0ea60f7c46491d87236822f6de35a6)](https://www.codacy.com/app/zhangdoa/InnocenceEngine?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=zhangdoa/InnocenceEngine&amp;utm_campaign=Badge_Grade)
[![Build status](https://ci.appveyor.com/api/projects/status/hl31o0q6nbmlf83i?svg=true)](https://ci.appveyor.com/project/zhangdoa/innocenceengine)
[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine.svg?type=shield)](https://app.fossa.io/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine?ref=badge_shield)
[![Blog](https://img.shields.io/badge/My-Blog-ff884d.svg)](http://zhangdoa.com/)
[![Twitter Follow](https://img.shields.io/twitter/follow/espadrine.svg?style=social&label=Follow)](https://twitter.com/zhangdoa)
> "A poet once said, 'The whole universe is in a glass of wine.'"
> -- Richard Feynman, 1963

## How to build

Currently avaliable platforms:

### Windows:

Windows 10 version 1803:

Prerequisites: Visual Studio 2017 + CMake 3.10 or higher

Run buildWin64VS15.bat or buildWin32VS15.bat to generate project and build, then run postBuildWin64.bat or postBuildWin32.bat to copy the resources files and dynamic libs for debug usage.

### Linux:

Ubuntu 18.04 LTS:

Prerequisites: CMake 3.10 or higher + OpenGL library(lGL) + GLFW3 library(lglfw3)

Run buildLinux64.sh to generate makefiles or buildLinux64-CodeBlocks.sh to get CodeBlocks IDE project, then manually make and build.

### Mac OSX:

version 10.13.6:

Prerequisites: CMake 3.10 or higher + XCode 9.4.1 or higher

Run buildMac64-XCode.sh to generate XCode projects and then manually build.

## Features

### Architecture

[Entity–component–system](https://en.wikipedia.org/wiki/Entity%E2%80%93component%E2%80%93system)


### Rendering

[Deferred shading](https://en.wikipedia.org/wiki/Deferred_shading)

[Physically based rendering](https://en.wikipedia.org/wiki/Physically_based_rendering)

[Scene graph](https://en.wikipedia.org/wiki/Scene_graph)


## References& Dependencies

### Third-party libraries

[GLFW](https://github.com/glfw/glfw)

[GLAD](https://github.com/Dav1dde/glad) 

[stb](https://github.com/nothings/stb)

[assimp](https://github.com/assimp)

[tinyXML2](https://github.com/leethomason/tinyxml2)


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

[Game Engine Architecture (1st Edition)](https://www.amazon.com/Game-Engine-Architecture-Jason-Gregory/dp/1568814135)

[Game Programming Patterns](https://www.amazon.com/Game-Programming-Patterns-Robert-Nystrom/dp/0990582906)

[Computer Graphics with Open GL (4th Edition)](https://www.amazon.com/Computer-Graphics-Open-GL-4th/dp/0136053580)

[Physically Based Rendering : From Theory to Implementation(2nd Edition)](https://www.amazon.com/Physically-Based-Rendering-Second-Implementation/dp/0123750792)


### Online tutorials& resources

[thebennybox's YouTube channel](https://www.youtube.com/user/thebennybox)

[Learn OpenGL](https://learnopengl.com)

[Scratchapixel](https://www.scratchapixel.com)

[Randy Gaul's Game Programming Blog](http://www.randygaul.net)

[Sébastien Lagarde's blog](https://seblagarde.wordpress.com)

[Stephen Hill's blog](http://blog.selfshadow.com)


## License
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine.svg?type=large)](https://app.fossa.io/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine?ref=badge_large)
