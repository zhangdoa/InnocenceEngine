# ECS Phase 4 — System Cleanup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Consolidate shadow cascade (CSM) computation into LightDataService, strip dead coupling from CameraSystem and LightSystem, rename LightSystem to LightSimulationService, and split AnimationService into resource-management and simulation halves.

**Architecture:** LightSystem currently owns CSM computation (view/projection matrices) that it exposes via accessor methods to LightDataService. After this phase, LightDataService owns the full CSM pipeline and reads only the raw 8-vertex frustum from CameraComponent, eliminating the accessor indirection. Simultaneously, AnimationService is split into AnimationResourceService (GPU allocation/upload) and AnimationSimulationService (play state + tick), matching the service-owns-operation-domain principle.

**Tech Stack:** C++17, InnocenceEngine ECS (EntityRegistry/ComponentStorage), DX12 via IRenderingServer, HLSL shadow maps

---

## Scope Check

Phase 4 has two independent subsystems:

1. **CSM Consolidation** (Tasks 1–3) — LightSimulationService + CameraSystem strip + LightDataService absorbs CSM
2. **Animation Split** (Task 4) — AnimationResourceService + AnimationSimulationService

These are independent and can fail separately. They share only the Engine.cpp registration step. If one is blocked, the other can still ship.

---

## File Structure

### CSM Consolidation

| File | Action | Responsibility |
|------|--------|----------------|
| `Source/Engine/Services/LightSimulationService.h` | Create | Color temperature + attenuation only; no CSM |
| `Source/Engine/Services/LightSimulationService.cpp` | Create | UpdateColorTemperature + UpdateAttenuationRadius |
| `Source/Engine/Services/LightSystem.h` | Delete | Replaced by LightSimulationService |
| `Source/Engine/Services/LightSystem.cpp` | Delete | Replaced by LightSimulationService |
| `Source/Engine/Component/CameraComponent.h` | Modify | Replace `m_SplitFrustumVerticesWS` with `m_FrustumVerticesWS` (8 full-frustum vertices) |
| `Source/Engine/Services/CameraSystem.cpp` | Modify | Remove GenerateCSMSplitFactors/SplitVertices/m_CSMSplitFactors; write `m_FrustumVerticesWS` |
| `Source/Engine/Services/LightDataService.cpp` | Modify | Absorb SnapAABBToShadowMap + AlignMatrixToTexels + full CSM computation; drop LightSystem dependency |
| `Source/Engine/Engine.cpp` | Modify | Replace LightSystem with LightSimulationService everywhere |

### Animation Split

| File | Action | Responsibility |
|------|--------|----------------|
| `Source/Engine/Services/AnimationSimulationService.h` | Create | AnimationData/AnimationInstance types; Play/Stop/Tick; RegisterAnimationData |
| `Source/Engine/Services/AnimationSimulationService.cpp` | Create | simulateAnimation tick; PlayAnimation/StopAnimation; m_AnimationDataInfosLUT |
| `Source/Engine/Services/AnimationResourceService.h` | Create | GPU skeleton/animation buffer management; InitializeAnimation; no play state |
| `Source/Engine/Services/AnimationResourceService.cpp` | Create | initializeAnimation GPU upload; calls AnimationSimulationService::RegisterAnimationData |
| `Source/Engine/Services/AnimationService.h` | Delete | Replaced by AnimationResourceService + AnimationSimulationService |
| `Source/Engine/Services/AnimationService.cpp` | Delete | Replaced by AnimationResourceService + AnimationSimulationService |
| `Source/Engine/Services/AnimationDrawCallService.h` | Modify | Include AnimationSimulationService.h instead of AnimationService.h |
| `Source/Engine/ThirdParty/JSONWrapper/JSONWrapper.cpp` | Modify | Remove stale `#include "../../Services/AnimationService.h"` (symbol unused) |
| `Source/DefaultClient/LogicClient/AnimationController.inl` | Modify | Replace `Get<AnimationService>` with `Get<AnimationSimulationService>` |
| `Source/Engine/Engine.cpp` | Modify | Rename AnimationService → AnimationResourceService; add AnimationSimulationService |

---

## Prerequisite: vcxproj Additions

The build project file lives in `Build/` (gitignored). After adding new `.cpp` files, manually add them in Visual Studio or the project file before building:

- Add `LightSimulationService.cpp` (remove `LightSystem.cpp`)
- Add `AnimationResourceService.cpp`, `AnimationSimulationService.cpp` (remove `AnimationService.cpp`)

---

## Task 1: Create LightSimulationService

**Goal:** Create a stripped-down replacement for LightSystem that only does per-light simulation (color temperature → RGB, attenuation radius). No CSM. No ComponentManager bridge.

**Files:**
- Create: `Source/Engine/Services/LightSimulationService.h`
- Create: `Source/Engine/Services/LightSimulationService.cpp`

- [ ] **Step 1: Write LightSimulationService.h**

```cpp
// Source/Engine/Services/LightSimulationService.h
#pragma once
#include "../Interface/ISystem.h"

namespace Inno
{
	class LightSimulationService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(LightSimulationService);

		bool Setup(ISystemConfig*) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
	};
}
```

- [ ] **Step 2: Write LightSimulationService.cpp**

```cpp
// Source/Engine/Services/LightSimulationService.cpp
#include "LightSimulationService.h"
#include "../Component/LightComponent.h"
#include "../Common/MathHelper.h"
#include "EntityRegistry.h"
#include "../Engine.h"

using namespace Inno;

bool LightSimulationService::Setup(ISystemConfig*)
{
	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool LightSimulationService::Initialize()
{
	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool LightSimulationService::Update()
{
	auto& l_Storage = g_Engine->Get<EntityRegistry>()->Storage<LightComponent>();
	auto& l_Lights = l_Storage.All();

	for (LightComponent& l_Light : l_Lights)
	{
		if (l_Light.m_UseColorTemperature)
			l_Light.m_RGBColor = Math::ColorTemperatureToRGB(l_Light.m_ColorTemperature);

		if (l_Light.m_LightType == LightType::Point)
		{
			auto l_NormalizedColor = l_Light.m_RGBColor.normalize();
			auto l_Luminance = 0.2126f * l_NormalizedColor.x + 0.7152f * l_NormalizedColor.y + 0.0722f * l_NormalizedColor.z;
			l_Light.m_Shape.x = std::sqrtf(l_Light.m_LuminousFlux * l_Luminance / (4.0f * PI<float> * 0.03f));
		}
	}

	return true;
}

bool LightSimulationService::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus LightSimulationService::GetStatus()
{
	return m_ObjectStatus;
}
```

- [ ] **Step 3: Build to confirm new files compile**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:Engine" 2>&1
```

Expected: Build succeeds (new files compile). LightSystem still exists at this point so no linker errors.

- [ ] **Step 4: Commit**

```
git add Source/Engine/Services/LightSimulationService.h
git add Source/Engine/Services/LightSimulationService.cpp
git commit -m "feat: add LightSimulationService — color temperature and attenuation only"
```

---

## Task 2: Replace CameraComponent.m_SplitFrustumVerticesWS with m_FrustumVerticesWS

**Goal:** Replace the pre-split frustum vertex array on CameraComponent with a fixed 8-vertex array representing the raw full frustum in world space. CameraSystem writes this; LightDataService reads it to compute splits itself.

**Files:**
- Modify: `Source/Engine/Component/CameraComponent.h`
- Modify: `Source/Engine/Services/CameraSystem.cpp`

- [ ] **Step 1: Modify CameraComponent.h**

Replace line 25 (`std::vector<Vertex> m_SplitFrustumVerticesWS;`) with:

```cpp
std::array<Vertex, 8> m_FrustumVerticesWS = {};
```

The full `CameraComponent` struct after change:
```cpp
struct CameraComponent
{
	static uint32_t GetTypeID() { return 4; };
	static const char* GetTypeName() { return "CameraComponent"; };

	Mat4 m_ProjectionMatrix = {};
	Frustum m_Frustum = {};
	Ray m_RayOfEye = {};
	float m_FOVX = 90.0f;
	float m_WidthScale = 16.0f;
	float m_HeightScale = 9.0f;
	float m_ZNear = 0.001f;
	float m_ZFar = 1000.0f;
	float m_WHRatio = 16.0f / 9.0f;
	float m_Aperture = 2.2f;
	float m_ShutterTime = 1.0f / 2000.0f;
	float m_ISO = 100.0f;

	std::array<Vertex, 8> m_FrustumVerticesWS = {};
};
```

- [ ] **Step 2: Strip CameraSystem.cpp — remove CSM generation**

In `CameraSystem.cpp`, make the following changes:

**Remove** the forward declarations from the namespace block (lines 18–30):
```cpp
// REMOVE these:
void GenerateCSMSplitFactors(float lambda = 0.75f);
void SplitVertices(const std::vector<Vertex>& frustumsVertices, const std::vector<float>& splitFactors, std::vector<Vertex> &splitVertices);
const uint32_t m_MaxCSMCount = 4;
std::vector<float> m_CSMSplitFactors;
```

**Remove** the full `GenerateCSMSplitFactors` function body (lines 33–47).

**Remove** the full `SplitVertices` function body (lines 56–109).

**In `GenerateFrustum`**, replace the last two lines:
```cpp
// REMOVE:
cameraComponent->m_SplitFrustumVerticesWS.resize(m_CSMSplitFactors.size() * 8);
SplitVertices(l_frustumVerticesWS, m_CSMSplitFactors, cameraComponent->m_SplitFrustumVerticesWS);

// ADD:
for (size_t i = 0; i < 8; ++i)
    cameraComponent->m_FrustumVerticesWS[i] = l_frustumVerticesWS[i];
```

**In `Update()`**, remove the `GenerateCSMSplitFactors()` call (line 154):
```cpp
// REMOVE:
GenerateCSMSplitFactors();
```

The updated `CameraSystem.cpp` namespace block after changes:
```cpp
namespace CameraSystemNS
{
	const size_t m_MaxComponentCount = 32;

	void GenerateProjectionMatrix(CameraComponent* cameraComponent);
	void GenerateFrustum(CameraComponent* cameraComponent, EntityID EntityID);
	void GenerateRayOfEye(CameraComponent* cameraComponent, EntityID EntityID);

	CameraComponent* m_MainCamera;
	CameraComponent* m_ActiveCamera;

	ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
}
```

The updated `GenerateFrustum` function:
```cpp
void CameraSystemNS::GenerateFrustum(CameraComponent* cameraComponent, EntityID EntityID)
{
	auto l_pCamera = cameraComponent->m_ProjectionMatrix;
	auto l_frustumVerticesVS = Math::GenerateFrustumInViewSpace(l_pCamera);

	auto* l_Transform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(EntityID);
	auto l_rCamera = l_Transform ? Math::toRotationMatrix(l_Transform->m_LocalRot) : Mat4();
	auto l_tCamera = l_Transform ? Math::toTranslationMatrix(Vec4(l_Transform->m_LocalPos, 1.0f)) : Mat4();
	auto l_frustumVerticesWS = Math::ViewToWorldSpace(l_frustumVerticesVS, l_tCamera, l_rCamera);
	cameraComponent->m_Frustum = Math::ToFrustum(&l_frustumVerticesWS[0]);

	for (size_t i = 0; i < 8; ++i)
		cameraComponent->m_FrustumVerticesWS[i] = l_frustumVerticesWS[i];
}
```

The updated `Update()` function:
```cpp
bool CameraSystem::Update()
{
	if (!m_MainCamera)
		return true;

	auto& l_Storage = g_Engine->Get<EntityRegistry>()->Storage<CameraComponent>();
	auto& l_Cameras = l_Storage.All();
	const auto& l_Owners = l_Storage.AllOwners();

	for (size_t i = 0; i < l_Cameras.size(); i++)
	{
		CameraComponent& l_Camera = l_Cameras[i];
		EntityID l_EntityID = l_Owners[i];

		l_Camera.m_WHRatio = l_Camera.m_WidthScale / l_Camera.m_HeightScale;
		GenerateProjectionMatrix(&l_Camera);
		GenerateRayOfEye(&l_Camera, l_EntityID);
		GenerateFrustum(&l_Camera, l_EntityID);
	}
	return true;
}
```

- [ ] **Step 3: Build to confirm**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:Engine" 2>&1
```

Expected: Build succeeds. LightSystem.cpp still reads `m_SplitFrustumVerticesWS` so it will fail — **this is expected**. We fix it in the next task.

Actually, to get a clean intermediate build, also check if LightSystem.cpp references m_SplitFrustumVerticesWS:
```
LightSystem.cpp line 87: auto& l_splitFrustumVerticesWS = l_cameraComponent->m_SplitFrustumVerticesWS;
```
This will fail. Proceed directly to Task 3 before building. (Or just note the expected error.)

- [ ] **Step 4: Commit (after Task 3 below builds cleanly)**

Hold this commit until after Task 3 so the tree stays buildable.

---

## Task 3: LightDataService Absorbs CSM Pipeline + Delete LightSystem

**Goal:** Move all CSM computation (SnapAABBToShadowMap, AlignMatrixToTexels, split factor generation, frustum splitting, AABB → light-space view/projection) into LightDataService. Remove LightSystem entirely.

**Files:**
- Modify: `Source/Engine/Services/LightDataService.cpp`
- Modify: `Source/Engine/Engine.cpp`
- Delete: `Source/Engine/Services/LightSystem.h`
- Delete: `Source/Engine/Services/LightSystem.cpp`

- [ ] **Step 1: Rewrite LightDataService.cpp**

Replace the top section (includes + LightDataServiceImpl struct) with:

```cpp
#include "LightDataService.h"

#include "../Common/LogService.h"
#include "../Common/MathHelper.h"
#include "../Common/GPUDataStructure.h"
#include "EntityRegistry.h"
#include "CameraSystem.h"
#include "RenderingConfigurationService.h"
#include "../Component/LightComponent.h"
#include "../Component/TransformComponent.h"
#include "../Component/CameraComponent.h"
#include "../Engine.h"

using namespace Inno;

namespace
{
	AABB SnapAABBToShadowMap(const AABB& Rhs, float ShadowMapResolution)
	{
		Vec4 l_UnitsPerTexel = Rhs.m_extend / ShadowMapResolution;
		Vec4 l_TexelPerUnit  = l_UnitsPerTexel.reciprocal();

		Vec4 l_SnappedCenter = Rhs.m_center.scale(l_TexelPerUnit) + 0.5f;
		l_SnappedCenter = Vec4(floor(l_SnappedCenter.x), floor(l_SnappedCenter.y), floor(l_SnappedCenter.z), 1.0f);
		l_SnappedCenter = l_SnappedCenter.scale(l_UnitsPerTexel);

		AABB l_Result;
		l_Result.m_center   = l_SnappedCenter;
		l_Result.m_extend   = Rhs.m_extend;
		l_Result.m_boundMin = l_Result.m_center - l_Result.m_extend * 0.5f;
		l_Result.m_boundMax = l_Result.m_center + l_Result.m_extend * 0.5f;
		return l_Result;
	}

	void AlignMatrixToTexels(Mat4& Matrix, float ShadowMapResolution)
	{
		Matrix.m30 = floor(Matrix.m30 * ShadowMapResolution) / ShadowMapResolution;
		Matrix.m31 = floor(Matrix.m31 * ShadowMapResolution) / ShadowMapResolution;
	}
}

namespace Inno
{
	struct LightDataServiceImpl
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		std::vector<PointLightConstantBuffer>  m_PointLightCBVector;
		std::vector<SphereLightConstantBuffer> m_SphereLightCBVector;
		std::vector<CSMConstantBuffer>         m_CSMCBVector;

		GPUBufferComponent* m_PointLightGPUBufferComp   = nullptr;
		GPUBufferComponent* m_SphereLightGPUBufferComp  = nullptr;
		GPUBufferComponent* m_CSMGPUBufferComp          = nullptr;
		GPUBufferComponent* m_GICBufferGPUBufferComp    = nullptr;

		bool Setup(ISystemConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();

		bool UpdateLightData();
		bool UpdateCSMData();
	};
}
```

The `Setup` and `Initialize` methods are unchanged. Replace `UpdateCSMData` with:

```cpp
bool LightDataServiceImpl::UpdateCSMData()
{
	auto& l_LightStorage = g_Engine->Get<EntityRegistry>()->Storage<LightComponent>();
	const auto& l_Lights  = l_LightStorage.All();
	const auto& l_LightOwners = l_LightStorage.AllOwners();

	if (l_Lights.empty())
		return false;

	EntityID l_SunEntityID = INVALID_ENTITY;
	for (size_t i = 0; i < l_Lights.size(); i++)
	{
		if (l_Lights[i].m_LightType == LightType::Directional)
		{
			l_SunEntityID = l_LightOwners[i];
			break;
		}
	}
	if (l_SunEntityID == INVALID_ENTITY)
		return false;

	auto* l_SunTransform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(l_SunEntityID);
	if (!l_SunTransform)
		return false;

	auto* l_Camera = static_cast<ICameraSystem*>(g_Engine->Get<CameraSystem>())->GetMainCamera();
	if (!l_Camera)
		return false;

	const uint32_t l_MaxCSMCount = 4;
	const float    l_Lambda      = 0.75f;
	const float    l_ZNear       = l_Camera->m_ZNear;
	const float    l_ZFar        = l_Camera->m_ZFar;

	std::array<float, 4> l_SplitFactors;
	for (int i = 1; i <= (int)l_MaxCSMCount; i++)
	{
		float l_Log     = l_ZNear * std::pow(l_ZFar / l_ZNear, (float)i / (float)l_MaxCSMCount);
		float l_Uniform = l_ZNear + (l_ZFar - l_ZNear) * ((float)i / (float)l_MaxCSMCount);
		l_SplitFactors[i - 1] = l_Log * l_Lambda + l_Uniform * (1.0f - l_Lambda);
	}

	const auto& l_FrustumWS = l_Camera->m_FrustumVerticesWS;

	// l_CornerPos layout:
	//   [0..3]           — near-plane corners (shared across all cascades)
	//   [4 + i*4 + j]    — far-plane corner j of cascade i  (i in [0,3], j in [0,3])
	//   total: 4 + 4*4 = 20 entries
	std::array<Vec3, 20> l_CornerPos;
	for (size_t i = 0; i < 4; i++)
		l_CornerPos[i] = l_FrustumWS[i].m_pos;
	for (size_t i = 0; i < l_MaxCSMCount; i++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			auto l_Dir = (l_FrustumWS[j + 4].m_pos - l_FrustumWS[j].m_pos).normalize();
			l_CornerPos[4 + i * 4 + j] = l_FrustumWS[j].m_pos + l_Dir * l_SplitFactors[i];
		}
	}

	auto l_RenderingConfig = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig();
	auto l_ShadowMapRes    = (float)l_RenderingConfig.shadowMapResolution;
	auto l_RotInv          = Math::toRotationMatrix(l_SunTransform->m_LocalRot).inverse();

	m_CSMCBVector.clear();
	for (size_t i = 0; i < l_MaxCSMCount; i++)
	{
		// Assemble 8 corners for this cascade:
		//   [0..3] near = l_CornerPos[0..3] (CSMFitToScene) or l_CornerPos[4 + (i-1)*4 .. ] (fit-to-cascade)
		//   [4..7] far  = l_CornerPos[4 + i*4 + 0..3]  for all modes
		std::array<Vertex, 8> l_CascadeVerts;
		if (l_RenderingConfig.CSMFitToScene)
		{
			// Near plane same for all cascades (full scene near)
			for (size_t j = 0; j < 4; j++)
				l_CascadeVerts[j].m_pos = l_CornerPos[j];
			// Far plane is the split plane for cascade i
			for (size_t j = 0; j < 4; j++)
				l_CascadeVerts[j + 4].m_pos = l_CornerPos[4 + i * 4 + j];
		}
		else
		{
			// Near plane = far plane of previous cascade (or camera near for cascade 0)
			// Near verts are at [4 + (i-1)*4 + j] for i>0, or [0..3] for i==0
			if (i == 0)
			{
				for (size_t j = 0; j < 4; j++)
					l_CascadeVerts[j].m_pos = l_CornerPos[j];
			}
			else
			{
				for (size_t j = 0; j < 4; j++)
					l_CascadeVerts[j].m_pos = l_CornerPos[4 + (i - 1) * 4 + j];
			}
			for (size_t j = 0; j < 4; j++)
				l_CascadeVerts[j + 4].m_pos = l_CornerPos[4 + i * 4 + j];
		}

		AABB l_AABBWorld = Math::GenerateAABB(&l_CascadeVerts[0], 8);
		AABB l_AABBLight = Math::ExtendAABBToBoundingSphere(l_AABBWorld);
		l_AABBLight = Math::RotateAABBToNewSpace(l_AABBLight, l_RotInv);
		l_AABBLight = SnapAABBToShadowMap(l_AABBLight, l_ShadowMapRes);

		Mat4 l_View = l_RotInv;
		AlignMatrixToTexels(l_View, l_ShadowMapRes);

		Mat4 l_Proj = Math::GenerateOrthographicMatrix(
			l_AABBLight.m_boundMin.x, l_AABBLight.m_boundMax.x,
			l_AABBLight.m_boundMin.y, l_AABBLight.m_boundMax.y,
			l_AABBLight.m_boundMax.z, l_AABBLight.m_boundMin.z);

		CSMConstantBuffer l_CB;
		l_CB.v       = l_View;
		l_CB.p       = l_Proj;
		l_CB.AABBMax = l_AABBWorld.m_boundMax;
		l_CB.AABBMin = l_AABBWorld.m_boundMin;
		m_CSMCBVector.emplace_back(l_CB);
	}

	return true;
}
```

The `UpdateLightData`, `Update`, `Terminate`, and all public method wrappers are unchanged.

- [ ] **Step 2: Update Engine.cpp — replace LightSystem with LightSimulationService**

In `Source/Engine/Engine.cpp`:

a. Replace include:
```cpp
// REMOVE:
#include "Services/LightSystem.h"
// ADD:
#include "Services/LightSimulationService.h"
```

b. Replace all occurrences of `LightSystem` with `LightSimulationService`:
- `Get<LightSystem>()` → `Get<LightSimulationService>()`
- `SystemSetup(LightSystem)` → `SystemSetup(LightSimulationService)`
- `SystemInit(LightSystem)` → `SystemInit(LightSimulationService)`
- `SystemTerm(LightSystem)` → `SystemTerm(LightSimulationService)`

The `Get<LightSystem>()->Update()` call at line 509 becomes `Get<LightSimulationService>()->Update()`.

- [ ] **Step 3: Confirm ComponentManager bridge is safe to delete**

`LightSystem::Setup` contains:
```cpp
g_Engine->Get<ComponentManager>()->RegisterType<LightComponent>(m_Impl->m_MaxComponentCount, this);
```
This was a Phase 2 migration bridge. Phase 2 migrated all LightComponent consumers to EntityRegistry. To confirm no ComponentManager consumers of LightComponent remain, run:
```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine && findstr /r /s \"ComponentManager.*LightComponent\|GetAll.*LightComponent\|Find.*LightComponent\" Source\*.cpp Source\*.h" 2>&1
```
Expected: No results (only the LightSystem.cpp line we are deleting). The bridge is dead code and the RegisterType call disappears with LightSystem — no replacement needed.

- [ ] **Step 4: Delete LightSystem files**

```bash
git rm Source/Engine/Services/LightSystem.h
git rm Source/Engine/Services/LightSystem.cpp
```

- [ ] **Step 5: Add LightSimulationService.cpp to vcxproj**

In Visual Studio, open the project file and:
- Remove `LightSystem.cpp` from the compile list
- Add `LightSimulationService.cpp`

(Or manually edit `Build/Source/Engine/Services/Services.vcxproj` to swap the ClCompile entry.)

- [ ] **Step 6: Full rebuild**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:Rebuild" 2>&1
```

Expected: Clean build with zero errors. LightSystem references are gone; LightSimulationService compiles; LightDataService compiles with the new CSM pipeline.

- [ ] **Step 7: Run tests**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
```

Expected: All EntityRegistry unit tests pass (the pre-existing crash on shutdown is known-unrelated).

- [ ] **Step 8: GPU validation**

```
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode" 2>&1
```

Expected: Exit code 0. Shadows must still render (CSM pipeline functionally equivalent to before).

- [ ] **Step 9: Commit all CSM consolidation changes**

```
git add Source/Engine/Services/LightSimulationService.h
git add Source/Engine/Services/LightSimulationService.cpp
git add Source/Engine/Component/CameraComponent.h
git add Source/Engine/Services/CameraSystem.cpp
git add Source/Engine/Services/LightDataService.cpp
git add Source/Engine/Engine.cpp
git commit -m "refactor: CSM consolidation — LightSystem→LightSimulationService, LightDataService absorbs shadow cascade pipeline"
```

---

## Task 4: AnimationSimulationService Split

**Goal:** Split AnimationService into (1) AnimationResourceService — GPU allocation/upload only — and (2) AnimationSimulationService — play state, tick, PlayAnimation/StopAnimation. Callers of PlayAnimation in AnimationController.inl are updated.

**Design note:** AnimationSimulationService owns `AnimationData`, `AnimationInstance`, `m_AnimationDataInfosLUT`, and `m_AnimationInstanceMap`. AnimationResourceService calls `AnimationSimulationService::RegisterAnimationData` after GPU init, bridging the two services cleanly.

**Files:**
- Create: `Source/Engine/Services/AnimationSimulationService.h`
- Create: `Source/Engine/Services/AnimationSimulationService.cpp`
- Create: `Source/Engine/Services/AnimationResourceService.h`
- Create: `Source/Engine/Services/AnimationResourceService.cpp`
- Modify: `Source/Engine/Services/AnimationDrawCallService.h`
- Modify: `Source/DefaultClient/LogicClient/AnimationController.inl`
- Modify: `Source/Engine/Engine.cpp`
- Delete: `Source/Engine/Services/AnimationService.h`
- Delete: `Source/Engine/Services/AnimationService.cpp`

- [ ] **Step 1: Write AnimationSimulationService.h**

```cpp
// Source/Engine/Services/AnimationSimulationService.h
#pragma once
#include "../Interface/ISystem.h"
#include "../Component/AnimationComponent.h"
#include "../Common/EntityID.h"

namespace Inno { struct GPUBufferComponent; }

namespace Inno
{
	struct AnimationData
	{
		AnimationComponent* ADC  = nullptr;
		GPUBufferComponent* keyData = nullptr;
	};

	struct AnimationInstance
	{
		AnimationData animationData;
		float         currentTime = 0.f;
		bool          isLooping   = false;
		bool          isFinished  = false;
	};

	struct AnimationSimulationServiceImpl;
	class AnimationSimulationService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(AnimationSimulationService);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		void RegisterAnimationData(const char* Name, AnimationData Data);

		bool           PlayAnimation(EntityID Entity, const char* AnimationName, bool IsLooping);
		bool           StopAnimation(EntityID Entity);
		AnimationInstance GetAnimationInstance(EntityID Entity);

	private:
		AnimationSimulationServiceImpl* m_Impl;
	};
}
```

- [ ] **Step 2: Write AnimationSimulationService.cpp**

```cpp
// Source/Engine/Services/AnimationSimulationService.cpp
#include "AnimationSimulationService.h"
#include "../Common/ThreadSafeUnorderedMap.h"
#include "../Common/Timer.h"
#include "../Engine.h"

using namespace Inno;

namespace Inno
{
	struct AnimationSimulationServiceImpl
	{
		void Tick();

		ThreadSafeUnorderedMap<std::string, AnimationData>    m_AnimationDataLUT;
		ThreadSafeUnorderedMap<EntityID, AnimationInstance>   m_AnimationInstanceMap;

		int64_t      m_PreviousTime = 0;
		int64_t      m_CurrentTime  = 0;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
	};
}

void AnimationSimulationServiceImpl::Tick()
{
	m_CurrentTime = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);
	float l_DeltaSec = float(m_CurrentTime - m_PreviousTime) / 1000.0f;

	for (auto& i : m_AnimationInstanceMap)
	{
		if (i.second.isFinished)
			continue;

		if (i.second.currentTime < i.second.animationData.ADC->m_Duration)
		{
			i.second.currentTime += l_DeltaSec / 60.0f;
		}
		else
		{
			if (i.second.isLooping)
				i.second.currentTime -= i.second.animationData.ADC->m_Duration;
			else
				i.second.isFinished = true;
		}
	}

	m_AnimationInstanceMap.erase_if([](auto it) { return it.second.isFinished; });
	m_PreviousTime = m_CurrentTime;
}

bool AnimationSimulationService::Setup(ISystemConfig*)
{
	m_Impl = new AnimationSimulationServiceImpl();
	m_Impl->m_PreviousTime = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);
	m_Impl->m_CurrentTime  = m_Impl->m_PreviousTime;
	m_Impl->m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool AnimationSimulationService::Initialize()
{
	m_Impl->m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool AnimationSimulationService::Update()
{
	m_Impl->Tick();
	return true;
}

bool AnimationSimulationService::Terminate()
{
	m_Impl->m_ObjectStatus = ObjectStatus::Terminated;
	delete m_Impl;
	return true;
}

ObjectStatus AnimationSimulationService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

void AnimationSimulationService::RegisterAnimationData(const char* Name, AnimationData Data)
{
	m_Impl->m_AnimationDataLUT.emplace(Name, Data);
}

bool AnimationSimulationService::PlayAnimation(EntityID Entity, const char* AnimationName, bool IsLooping)
{
	auto l_It = m_Impl->m_AnimationDataLUT.find(AnimationName);
	if (l_It == m_Impl->m_AnimationDataLUT.end())
		return false;

	AnimationInstance l_Instance;
	l_Instance.animationData = l_It->second;
	l_Instance.currentTime   = 0.0f;
	l_Instance.isLooping     = IsLooping;
	l_Instance.isFinished    = false;
	m_Impl->m_AnimationInstanceMap.emplace(Entity, l_Instance);
	return true;
}

bool AnimationSimulationService::StopAnimation(EntityID Entity)
{
	auto l_It = m_Impl->m_AnimationInstanceMap.find(Entity);
	if (l_It == m_Impl->m_AnimationInstanceMap.end())
		return false;
	m_Impl->m_AnimationInstanceMap.erase(l_It->first);
	return true;
}

AnimationInstance AnimationSimulationService::GetAnimationInstance(EntityID Entity)
{
	auto l_It = m_Impl->m_AnimationInstanceMap.find(Entity);
	if (l_It != m_Impl->m_AnimationInstanceMap.end())
		return l_It->second;
	return AnimationInstance{};
}
```

- [ ] **Step 3: Write AnimationResourceService.h**

```cpp
// Source/Engine/Services/AnimationResourceService.h
#pragma once
#include "../Interface/ISystem.h"
#include "../Component/SkeletonComponent.h"
#include "../Component/AnimationComponent.h"
#include "../Common/EntityID.h"

namespace Inno
{
	struct AnimationResourceServiceImpl;
	class AnimationResourceService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(AnimationResourceService);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		SkeletonComponent* AddSkeletonComponent();
		AnimationComponent* AddAnimationComponent();

		bool InitializeSkeletonComponent(SkeletonComponent* rhs);
		bool InitializeAnimationComponent(AnimationComponent* rhs);

	private:
		AnimationResourceServiceImpl* m_Impl;
	};
}
```

- [ ] **Step 4: Write AnimationResourceService.cpp**

```cpp
// Source/Engine/Services/AnimationResourceService.cpp
#include "AnimationResourceService.h"
#include "AnimationSimulationService.h"
#include "../Common/ThreadSafeQueue.h"
#include "../Common/LogService.h"
#include "EntityRegistry.h"
#include "../Engine.h"

using namespace Inno;

namespace Inno
{
	struct AnimationResourceServiceImpl
	{
		void InitializeAnimation(AnimationComponent* rhs);

		ThreadSafeQueue<AnimationComponent*> m_UninitializedAnimations;

		uint32_t     m_SkeletonCount  = 0;
		uint32_t     m_AnimationCount = 0;
		ObjectStatus m_ObjectStatus   = ObjectStatus::Invalid;
	};
}

void AnimationResourceServiceImpl::InitializeAnimation(AnimationComponent* rhs)
{
	std::string l_Name = rhs->m_InstanceName.c_str();

	auto l_KeyData = g_Engine->getRenderingServer()->AddGPUBufferComponent((l_Name + "_KeyData").c_str());
	l_KeyData->m_ElementCount     = rhs->m_KeyData.capacity();
	l_KeyData->m_ElementSize      = sizeof(KeyData);
	l_KeyData->m_GPUAccessibility = Accessibility::ReadWrite;

	g_Engine->getRenderingServer()->Initialize(l_KeyData);
	g_Engine->getRenderingServer()->Upload(l_KeyData, &rhs->m_KeyData[0]);

	AnimationData l_Data;
	l_Data.ADC     = rhs;
	l_Data.keyData = l_KeyData;

	g_Engine->Get<AnimationSimulationService>()->RegisterAnimationData(rhs->m_InstanceName.c_str(), l_Data);
}

bool AnimationResourceService::Setup(ISystemConfig*)
{
	m_Impl = new AnimationResourceServiceImpl();
	m_Impl->m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool AnimationResourceService::Initialize()
{
	m_Impl->m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool AnimationResourceService::Update()
{
	AnimationComponent* l_Pending = nullptr;
	while (m_Impl->m_UninitializedAnimations.tryPop(l_Pending))
	{
		if (l_Pending)
			m_Impl->InitializeAnimation(l_Pending);
	}
	return true;
}

bool AnimationResourceService::Terminate()
{
	delete m_Impl;
	return true;
}

ObjectStatus AnimationResourceService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

SkeletonComponent* AnimationResourceService::AddSkeletonComponent()
{
	auto l_Entity = g_Engine->Get<EntityRegistry>()->Spawn(
		ObjectLifespan::Persistence, ("Skeleton_" + std::to_string(m_Impl->m_SkeletonCount) + "/").c_str());
	auto& l_SDC = g_Engine->Get<EntityRegistry>()->Emplace<SkeletonComponent>(l_Entity);
	m_Impl->m_SkeletonCount++;
	return &l_SDC;
}

AnimationComponent* AnimationResourceService::AddAnimationComponent()
{
	auto l_Entity = g_Engine->Get<EntityRegistry>()->Spawn(
		ObjectLifespan::Persistence, ("Animation_" + std::to_string(m_Impl->m_AnimationCount) + "/").c_str());
	auto& l_ADC = g_Engine->Get<EntityRegistry>()->Emplace<AnimationComponent>(l_Entity);
	m_Impl->m_AnimationCount++;
	return &l_ADC;
}

bool AnimationResourceService::InitializeSkeletonComponent(SkeletonComponent*)
{
	return true;
}

bool AnimationResourceService::InitializeAnimationComponent(AnimationComponent* rhs)
{
	m_Impl->m_UninitializedAnimations.push(rhs);
	return true;
}
```

- [ ] **Step 5: Update AnimationDrawCallService.h**

Replace:
```cpp
#include "AnimationService.h"
```
With:
```cpp
#include "AnimationSimulationService.h"
```

- [ ] **Step 6: Remove stale AnimationService include from JSONWrapper.cpp**

In `Source/Engine/ThirdParty/JSONWrapper/JSONWrapper.cpp`, delete line 8:
```cpp
#include "../../Services/AnimationService.h"
```
This include is a stale leftover — no AnimationService symbols are used in JSONWrapper.cpp.

- [ ] **Step 7: Update AnimationController.inl**

In `Source/DefaultClient/LogicClient/AnimationController.inl`:

Replace the `AnimationService.h` include at the top with:
```cpp
#include "../../Engine/Services/AnimationSimulationService.h"
```

Replace all `Get<AnimationService>()->PlayAnimation(...)` with `Get<AnimationSimulationService>()->PlayAnimation(...)`.

Replace all `Get<AnimationService>()->StopAnimation(...)` with `Get<AnimationSimulationService>()->StopAnimation(...)`.

- [ ] **Step 8: Update Engine.cpp — swap AnimationService for the two new services**

a. Replace includes:
```cpp
// REMOVE:
#include "Services/AnimationService.h"
// ADD:
#include "Services/AnimationResourceService.h"
#include "Services/AnimationSimulationService.h"
```

b. Replace `Get<AnimationService>()` → `Get<AnimationResourceService>()` for the resource registration location (line ~378 in Engine.cpp where services are retrieved for service-locator registration).

c. Swap `SystemSetup`/`SystemInit`/`SystemTerm`/`SystemUpdate` calls:
- `SystemSetup(AnimationService)` → `SystemSetup(AnimationResourceService)` + add `SystemSetup(AnimationSimulationService)` before it
- `SystemInit(AnimationService)` → `SystemInit(AnimationResourceService)` + add `SystemInit(AnimationSimulationService)` before it
- `SystemTerm(AnimationService)` → `SystemTerm(AnimationResourceService)` + add `SystemTerm(AnimationSimulationService)` after it (terminate simulation before resources)
- `Get<AnimationService>()->Update()` → `Get<AnimationResourceService>()->Update()` + add `Get<AnimationSimulationService>()->Update()` immediately after (or before — either order is fine since they're independent)

**Registration order note:** AnimationSimulationService must be set up before AnimationResourceService because `AnimationResourceService::InitializeAnimation` calls `g_Engine->Get<AnimationSimulationService>()`. Set up AnimationSimulationService first.

- [ ] **Step 9: Delete old AnimationService files**

```bash
git rm Source/Engine/Services/AnimationService.h
git rm Source/Engine/Services/AnimationService.cpp
```

- [ ] **Step 10: Update vcxproj**

- Remove `AnimationService.cpp`
- Add `AnimationResourceService.cpp`, `AnimationSimulationService.cpp`

- [ ] **Step 11: Full rebuild**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Build && msbuild InnocenceEngine.sln /p:Configuration=RelWithDebInfo /t:Rebuild" 2>&1
```

Expected: Zero errors. All references to AnimationService resolved.

- [ ] **Step 12: Run tests**

```
cmd.exe /c "cd C:\GitRepo\InnocenceEngine\Bin && RelWithDebInfo\Test.exe" 2>&1
```

Expected: All EntityRegistry tests pass.

- [ ] **Step 13: GPU validation**

```
powershell.exe -Command "Set-Location 'C:\GitRepo\InnocenceEngine\Bin'; (Start-Process -FilePath 'RelWithDebInfo\RenderTest.exe' -ArgumentList '-mode 0 -renderer 0 -loglevel 0 -offscreen -test draw_instanced' -Wait -PassThru -NoNewWindow).ExitCode" 2>&1
```

Expected: Exit code 0.

- [ ] **Step 14: Commit**

```
git add Source/Engine/Services/AnimationSimulationService.h
git add Source/Engine/Services/AnimationSimulationService.cpp
git add Source/Engine/Services/AnimationResourceService.h
git add Source/Engine/Services/AnimationResourceService.cpp
git add Source/Engine/Services/AnimationDrawCallService.h
git add Source/DefaultClient/LogicClient/AnimationController.inl
git add Source/Engine/Engine.cpp
git commit -m "refactor: split AnimationService into AnimationResourceService (GPU) and AnimationSimulationService (tick)"
```

---

## Verification Checklist

After all tasks complete:

- [ ] `msbuild /t:Rebuild` exits 0 with no errors or warnings about missing symbols
- [ ] `LightSystem.h` and `LightSystem.cpp` no longer exist in the repo
- [ ] `AnimationService.h` and `AnimationService.cpp` no longer exist in the repo
- [ ] `Test.exe` exits without any test failures
- [ ] `RenderTest.exe -test draw_instanced` exits 0 (shadows rendered correctly)
- [ ] `git grep LightSystem Source/` returns empty
- [ ] `git grep AnimationService Source/` returns empty (only the new service names appear)

---

## Known Pre-Existing Issue

`Test.exe` exits with code -1073741819 (0xC0000005) during shutdown. This crash is **pre-existing and unrelated to Phase 4**. It was present before Phase 3. Do not treat it as a regression.
