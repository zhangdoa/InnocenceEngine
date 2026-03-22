# Innocence Engine
![Screen capture](https://github.com/zhangdoa/InnocenceEngine/blob/master/ScreenCapture.jpg)
[![CodeFactor](https://www.codefactor.io/repository/github/zhangdoa/innocenceengine/badge)](https://www.codefactor.io/repository/github/zhangdoa/innocenceengine)
[![GPL-3.0 licensed](https://img.shields.io/badge/license-GPL--3.0-brightgreen.svg)](LICENSE.md)
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine.svg?type=shield)](https://app.fossa.io/projects/git%2Bgithub.com%2Fzhangdoa%2FInnocenceEngine?ref=badge_shield)
[![Blog](https://img.shields.io/badge/My-Blog-ff884d.svg)](http://zhangdoa.com/)
[![Twitter Follow](https://img.shields.io/twitter/follow/espadrine.svg?style=social&label=Follow)](https://twitter.com/zhangdoa)

> "A poet once said, 'The whole universe is in a glass of wine.'"
> -- Richard Feynman, 1963

## Architecture
![Architecture](https://github.com/zhangdoa/InnocenceEngine/blob/master/architecture.svg)

## Features

- **Entity–Component–System** architecture. Components are plain data containers; services own operation domains rather than component types.

```cpp
// Spawn an entity
EntityID l_entity = g_Engine->Get<EntityRegistry>()->Spawn("testEntity/");

// Add components
auto* l_transform = g_Engine->Get<EntityRegistry>()->Emplace<WorldTransformComponent>(l_entity);
l_transform->m_localTransformVector.m_pos = Vec4(42.0f, 1.0f, 0.0f, 1.0f);

auto* l_mesh = g_Engine->Get<EntityRegistry>()->Emplace<MeshComponent>(l_entity);
auto* l_material = g_Engine->Get<EntityRegistry>()->Emplace<MaterialComponent>(l_entity);
```

- Custom container, string and math classes — no STL overhead, no third-party math library.

```cpp
RingBuffer<float> l_testRingBuffer(32);
Array<float, true> l_testThreadSafeArray;  // thread-safe variant
FixedSizeString<64> l_testString;

l_testString = "Hello,World/";
for (size_t i = 0; i < l_testString.size(); i++)
    l_testRingBuffer.emplace_back((float)i);

auto l_maxPoint = Vec4(l_testRingBuffer[0], l_testRingBuffer[1], l_testRingBuffer[16], 1.0f);
auto l_testAABB = InnoMath::generateAABB(l_maxPoint, -l_maxPoint);
```

- Job-graph based parallel task model — fully utilises modern hardware, lock-free in client logic code.

```cpp
auto l_taskA = g_Engine->Get<TaskScheduler>()->Submit("TaskA", 5, nullptr, []() {
    Log(Warning, "Running on thread 5");
});

auto l_taskB = g_Engine->Get<TaskScheduler>()->Submit("TaskB", 2, l_taskA.m_Task,
    [](int val) { return val * 2; }, 21);

l_taskB.m_Task->Wait();
auto result = l_taskB.m_Future->Get();  // 42
```

- Object pool memory model — O(1) allocation and deallocation.

```cpp
auto l_pool = TObjectPool<MyPOD>::Create(65536);
auto* l_obj = l_pool->Spawn();
l_obj->m_Value = 42.0f;
l_pool->Destroy(l_obj);
TObjectPool<MyPOD>::Destruct(l_pool);
```

- Client-as-plugin rendering architecture — implement `IRenderingClient` to define your own pipeline from first draw call to swap chain presentation.

```cpp
auto l_renderingServer = g_Engine->getRenderingServer();

l_renderingServer->CommandListBegin(m_RPDC, 0);
l_renderingServer->BindRenderPassDataComponent(m_RPDC);
l_renderingServer->CleanRenderTargets(m_RPDC);
l_renderingServer->BindGPUResource(m_RPDC, ShaderStage::Pixel, m_SDC, 17);
l_renderingServer->DrawIndexedInstanced(m_RPDC, m_quadMesh);
l_renderingServer->CommandListEnd(m_RPDC);
l_renderingServer->ExecuteCommandList(m_RPDC);
l_renderingServer->WaitForFrame(m_RPDC);
```

- Physically-based lighting with photometry interface — colour temperature, luminous flux, and real-world light measurements.

- Default rendering client features:
  - Tiled-deferred rendering pipeline
  - Cook-Torrance BRDF (Disney diffuse + multi-scattering GGX specular)
  - OIT (order-independent transparency)
  - SSAO, CSM with VSM/PCF, procedural sky
  - Motion blur, TAA, ACES tone mapping
  - GPU-driven instanced rendering

- Real-time GI — baked PRT for large scenes, bake-free SVOGI for smaller scenes.

- Unified asset management using JSON for all text data with binary I/O support.

- Physics simulation via NVIDIA PhysX.

- Qt-based GUI editor.

## How to build?

### Windows

Tested OS: Windows 11

#### Prerequisites

- MSVC 17.x+
- CMake 3.26+

#### Build

```powershell
Scripts/BuildWin.ps1
Scripts/PostBuildWin.ps1
```

### Linux

Tested OS: Ubuntu 18.04 LTS

#### Prerequisites

- GCC 8.0 or Clang 7.0+
- CMake 3.10+
- OpenGL library (lGL)

#### Build

```shell
bash Scripts/SetupLinux.sh
bash Scripts/BuildAssimpLinux.sh
bash Scripts/BuildGLADLinux.sh
bash Scripts/BuildEngineLinux.sh
```

### macOS

Tested OS: macOS 10.13.6, 10.15.4

#### Prerequisites

- CMake 3.10+
- Apple Clang 10.0 or LLVM Clang 8.0+

#### Build

```shell
bash Scripts/SetupMac.sh
bash Scripts/BuildAssimpMac-Xcode.sh
bash Scripts/BuildGLADMac-Xcode.sh
bash Scripts/BuildEngineMac-Xcode.sh
bash Scripts/PostBuildMac.sh
```

## How to use?

1. Implement `ILogicClient` and `IRenderingClient` and place source files under `Source/Client/LogicClient` and `Source/Client/RenderingClient`
2. Set the CMake variables `INNO_LOGIC_CLIENT` and `INNO_RENDERING_CLIENT` in `Source/CMakeLists.txt` to your class names
3. Build and launch via `Bin/RelWithDebInfo/Main.exe` (Windows) or the equivalent on Linux/macOS

## How to debug

### Windows

1. Open the workspace folder in VSCode
2. Set debug launch arguments in `.vscode/launch.json`
3. Start debug with F5

### Linux

1. Load the working copy in your IDE of choice
2. Select build/makefile and hit compile and debug (F6 in Atom)

### macOS

1. Open `Build/InnocenceEngine.xcodeproj`
2. Select Product → Run (⌘R)

## How to bake a scene?

### Windows

```powershell
Scripts/BakeScene.ps1 -sceneName [scene file name without extension]
```

## Launch arguments

```
-mode [value]
```
| Value | Notes |
| --- | --- |
| 0 | Engine handles window creation and event management (normal game client) |
| 1 | Engine requires an external window handle from the client (editor mode) |

```
-renderer [value]
```
| Value | Backend | Notes |
| --- | --- | --- |
| 0 | OpenGL | Deprecated |
| 1 | DirectX 11 | Deprecated |
| 2 | DirectX 12 | Primary — Windows only |
| 3 | Vulkan | WIP |
| 4 | Metal | WIP — macOS only |

```
-loglevel [value]
```
| Value | Notes |
| --- | --- |
| 0 | Verbose and above |
| 1 | Success and above |
| 2 | Warning and above |
| 3 | Error only |

## Shader compilation

Requires [DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler/releases).

```powershell
Scripts/HLSL2DXIL.ps1   # DXIL for DirectX 12
Scripts/HLSL2SPIR-V.ps1  # SPIR-V for Vulkan
```

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

[Free3D](https://thefree3dmodels.com)
[Musopen](https://musopen.org)
[Free PBR Materials](https://freepbr.com/)
[HDR Labs](http://www.hdrlabs.com/)
