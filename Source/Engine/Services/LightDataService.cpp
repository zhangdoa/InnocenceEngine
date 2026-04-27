#include "LightDataService.h"

#include "../Common/LogService.h"
#include "../Common/MathHelper.h"
#include "../Common/GPUDataStructure.h"
#include "EntityRegistry.h"
#include "CameraService.h"
#include "RenderingConfigurationService.h"
#include "../Component/LightComponent.h"
#include "../Component/TransformComponent.h"
#include "../Component/CameraComponent.h"
#include "../Engine.h"
#include "GPUBufferResourceService.h"
#include "TextureResourceService.h"
#include "../Component/TextureComponent.h"

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

	uint32_t LookupAtlasSlot(const std::vector<uint32_t>& Slots, uint32_t Index, const char* Caller)
	{
		if (Index >= Slots.size())
		{
			Log(Warning, Caller, ": index ", Index, " out of range (count=", Slots.size(), ").");
			return INVALID_ATLAS_SLOT;
		}
		return Slots[Index];
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
		std::vector<PointShadowConstantBuffer> m_PointShadowCBVector;

		// Parallel-indexed to m_PointLightCBVector / m_SphereLightCBVector;
		// initialized to INVALID_ATLAS_SLOT in UpdateLightData, overwritten
		// by UpdatePointShadowData for shadow-casting lights.
		std::vector<uint32_t> m_PointLightAtlasSlot;
		std::vector<uint32_t> m_SphereLightAtlasSlot;

		GPUBufferComponent* m_PointLightGPUBufferComp   = nullptr;
		GPUBufferComponent* m_SphereLightGPUBufferComp  = nullptr;
		GPUBufferComponent* m_CSMGPUBufferComp          = nullptr;
		GPUBufferComponent* m_GICBufferGPUBufferComp    = nullptr;
		GPUBufferComponent* m_PointShadowGPUBufferComp  = nullptr;

		// TASK-66 / TASK-150 cube-atlas: Texture2DArray, R32G32B32A32_FLOAT,
		// DepthOrArraySize = maxPointShadows * 6. Allocated here so GBV is
		// validated on engine boot; TASK-148's PointShadowGeometryProcessPass
		// retrieves the texture via LightDataService::GetPointShadowAtlas() and
		// drives it as the render target via a custom RenderTargetsCreationFunc.
		// Lifetime: created in Setup, initialized in Initialize, deleted in
		// Terminate. No per-frame state mutation here — all transitions go
		// through the standard barrier path when TASK-148 binds it.
		TextureComponent*   m_PointShadowAtlas         = nullptr;

		bool Setup(IServiceConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();

		bool UpdateLightData();
		bool UpdateCSMData();
		// Slot allocator + cbuffer populator (TASK-147). Walks point/sphere lights
		// in deterministic storage order; for each m_CastShadow == true light,
		// assigns the next free atlas slot in [0..maxPointShadows-1] and writes
		// a PointShadowConstantBuffer entry with the 6 cube-face view matrices.
		// Lights beyond the budget keep INVALID_ATLAS_SLOT.
		bool UpdatePointShadowData();
	};
}

bool LightDataServiceImpl::Setup(IServiceConfig* systemConfig)
{
	auto l_rsService = g_Engine->Get<GPUBufferResourceService>();

	m_PointLightGPUBufferComp = l_rsService->Add("PointLightCBuffer");
	m_SphereLightGPUBufferComp = l_rsService->Add("SphereLightCBuffer");
	m_CSMGPUBufferComp = l_rsService->Add("CSMCBuffer");
	m_GICBufferGPUBufferComp = l_rsService->Add("GICBuffer");
	m_PointShadowGPUBufferComp = l_rsService->Add("PointShadowCBuffer");

	m_PointShadowAtlas = g_Engine->Get<TextureResourceService>()->Add("PointShadowAtlas");

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool LightDataServiceImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
	auto l_rsService = g_Engine->Get<GPUBufferResourceService>();
		auto l_RenderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

		m_PointLightGPUBufferComp->m_ElementCount = l_RenderingCapability.maxPointLights;
		m_PointLightGPUBufferComp->m_ElementSize = sizeof(PointLightConstantBuffer);

		l_rsService->Initialize(m_PointLightGPUBufferComp);

		m_SphereLightGPUBufferComp->m_ElementCount = l_RenderingCapability.maxSphereLights;
		m_SphereLightGPUBufferComp->m_ElementSize = sizeof(SphereLightConstantBuffer);

		l_rsService->Initialize(m_SphereLightGPUBufferComp);

		m_CSMGPUBufferComp->m_ElementCount = l_RenderingCapability.maxCSMSplits;
		m_CSMGPUBufferComp->m_ElementSize = sizeof(CSMConstantBuffer);

		l_rsService->Initialize(m_CSMGPUBufferComp);

		m_GICBufferGPUBufferComp->m_ElementSize = sizeof(GIConstantBuffer);
		m_GICBufferGPUBufferComp->m_ElementCount = 1;

		l_rsService->Initialize(m_GICBufferGPUBufferComp);

		m_PointShadowGPUBufferComp->m_ElementCount = l_RenderingCapability.maxPointShadows;
		m_PointShadowGPUBufferComp->m_ElementSize = sizeof(PointShadowConstantBuffer);

		l_rsService->Initialize(m_PointShadowGPUBufferComp);

		// Atlas resource: Texture2DArray, R32G32B32A32_FLOAT, DepthOrArraySize =
		// maxPointShadows * 6 — exact mirror of SunShadowGeometryProcessPass.cpp
		// render-target descriptor. Per design call (TASK-150), per-face is 256²
		// and the caster `frag` (TASK-148) writes (depth, depth², 0, 1).
		// IsMultiBuffer=true matches sun-shadow precedent for swap-chain cycling;
		// ColorAttachment usage routes through the RT bind-flag path
		// (DX12Helper::GetTextureBindFlags).
		const uint32_t l_PerFaceResolution = 256;
		const uint32_t l_AtlasSliceCount   = l_RenderingCapability.maxPointShadows * 6;
		m_PointShadowAtlas->m_TextureDesc.Sampler           = TextureSampler::Sampler2DArray;
		m_PointShadowAtlas->m_TextureDesc.Usage             = TextureUsage::ColorAttachment;
		m_PointShadowAtlas->m_TextureDesc.IsMultiBuffer     = true;
		m_PointShadowAtlas->m_TextureDesc.PixelDataFormat   = TexturePixelDataFormat::RGBA;
		m_PointShadowAtlas->m_TextureDesc.PixelDataType     = TexturePixelDataType::Float32;
		m_PointShadowAtlas->m_TextureDesc.Width             = l_PerFaceResolution;
		m_PointShadowAtlas->m_TextureDesc.Height            = l_PerFaceResolution;
		m_PointShadowAtlas->m_TextureDesc.DepthOrArraySize  = l_AtlasSliceCount;
		m_PointShadowAtlas->m_TextureDesc.MipLevels         = 1;
		// Match SunShadowGeometryProcessPass.cpp:49-59: clear/border to
		// (1, 1, 0, 1) so unrendered texels at far plane don't register as
		// blockers at depth 0 in PCSS.
		m_PointShadowAtlas->m_TextureDesc.BorderColor[0] = 1.0f;
		m_PointShadowAtlas->m_TextureDesc.BorderColor[1] = 1.0f;
		m_PointShadowAtlas->m_TextureDesc.BorderColor[2] = 0.0f;
		m_PointShadowAtlas->m_TextureDesc.BorderColor[3] = 1.0f;
		m_PointShadowAtlas->m_TextureDesc.ClearColor[0]  = 1.0f;
		m_PointShadowAtlas->m_TextureDesc.ClearColor[1]  = 1.0f;
		m_PointShadowAtlas->m_TextureDesc.ClearColor[2]  = 0.0f;
		m_PointShadowAtlas->m_TextureDesc.ClearColor[3]  = 1.0f;

		g_Engine->Get<TextureResourceService>()->Initialize(m_PointShadowAtlas);

		m_ObjectStatus = ObjectStatus::Activated;
		Log(Success, "LightDataService has been initialized.");
		return true;
	}
	else
	{
		Log(Error, "LightDataService is not created!");
		return false;
	}
}

bool LightDataServiceImpl::UpdateLightData()
{
	m_PointLightCBVector.clear();
	m_SphereLightCBVector.clear();
	m_PointLightAtlasSlot.clear();
	m_SphereLightAtlasSlot.clear();

	auto& l_Storage = g_Engine->Get<EntityRegistry>()->Storage<LightComponent>();
	const auto& l_Lights = l_Storage.All();
	const auto& l_Owners = l_Storage.AllOwners();

	if (l_Lights.empty())
		return false;

	for (size_t i = 0; i < l_Lights.size(); i++)
	{
		const LightComponent& l_Light = l_Lights[i];
		EntityID l_EntityID = l_Owners[i];
		auto* l_Transform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(l_EntityID);

		if (l_Light.m_LightType == LightType::Point)
		{
			PointLightConstantBuffer l_data;
			if (l_Transform)
				l_data.pos = l_Transform->m_LocalPos;
			l_data.luminance = l_Light.m_RGBColor * l_Light.m_LuminousFlux;
			l_data.luminance.w = l_Light.m_Shape.x;
			m_PointLightCBVector.emplace_back(l_data);
			m_PointLightAtlasSlot.emplace_back(INVALID_ATLAS_SLOT);
		}
		else if (l_Light.m_LightType == LightType::Sphere)
		{
			SphereLightConstantBuffer l_data;
			if (l_Transform)
				l_data.pos = l_Transform->m_LocalPos;
			l_data.luminance = l_Light.m_RGBColor * l_Light.m_LuminousFlux;
			l_data.luminance.w = l_Light.m_Shape.x;
			m_SphereLightCBVector.emplace_back(l_data);
			m_SphereLightAtlasSlot.emplace_back(INVALID_ATLAS_SLOT);
		}
	}

	return true;
}

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

	auto* l_Camera = static_cast<ICameraService*>(g_Engine->Get<CameraService>())->GetMainCamera();
	if (!l_Camera)
		return false;

	const uint32_t l_MaxCSMCount = 4;
	const float    l_Lambda      = 0.75f;
	const float    l_ZNear       = l_Camera->m_ZNear;
	const float    l_ZFar        = l_Camera->m_ZFar;
	if (l_ZFar <= l_ZNear)
		return false;

	std::array<float, 4> l_SplitFactors;
	for (int i = 1; i <= (int)l_MaxCSMCount; i++)
	{
		float l_Log     = l_ZNear * std::pow(l_ZFar / l_ZNear, (float)i / (float)l_MaxCSMCount);
		float l_Uniform = l_ZNear + (l_ZFar - l_ZNear) * ((float)i / (float)l_MaxCSMCount);
		l_SplitFactors[i - 1] = l_Log * l_Lambda + l_Uniform * (1.0f - l_Lambda);
	}

	const auto& l_FrustumWS = l_Camera->m_FrustumVerticesWS;

	// l_CornerPos layout:
	//   [0..3]        — near-plane corners (shared across all cascades)
	//   [4 + i*4 + j] — far-plane corner j of cascade i  (i in [0,3], j in [0,3])
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
		std::array<Vertex, 8> l_CascadeVerts;
		if (l_RenderingConfig.CSMFitToScene)
		{
			for (size_t j = 0; j < 4; j++)
				l_CascadeVerts[j].m_pos = l_CornerPos[j];
			for (size_t j = 0; j < 4; j++)
				l_CascadeVerts[j + 4].m_pos = l_CornerPos[4 + i * 4 + j];
		}
		else
		{
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

// UpdatePointShadowData lives in a sibling .inl to keep LightDataService.cpp
// under the file-size soft ratchet (.claude/disciplines/split-before-grow.md).
#include "LightDataService_PointShadow.inl"

bool LightDataServiceImpl::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		UpdateLightData();
		UpdateCSMData();
		UpdatePointShadowData();

	auto l_rsService = g_Engine->Get<GPUBufferResourceService>();

		if (m_PointLightCBVector.size() > 0)
		{
			l_rsService->Upload(m_PointLightGPUBufferComp, m_PointLightCBVector, 0, m_PointLightCBVector.size());
		}
		if (m_SphereLightCBVector.size() > 0)
		{
			l_rsService->Upload(m_SphereLightGPUBufferComp, m_SphereLightCBVector, 0, m_SphereLightCBVector.size());
		}
		if (m_CSMCBVector.size() > 0)
		{
			l_rsService->Upload(m_CSMGPUBufferComp, m_CSMCBVector, 0, m_CSMCBVector.size());
		}
		if (m_PointShadowCBVector.size() > 0)
		{
			l_rsService->Upload(m_PointShadowGPUBufferComp, m_PointShadowCBVector, 0, m_PointShadowCBVector.size());
		}

		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool LightDataServiceImpl::Terminate()
{
	auto l_rsService = g_Engine->Get<GPUBufferResourceService>();

	l_rsService->Delete(m_PointLightGPUBufferComp);
	l_rsService->Delete(m_SphereLightGPUBufferComp);
	l_rsService->Delete(m_CSMGPUBufferComp);
	l_rsService->Delete(m_GICBufferGPUBufferComp);
	l_rsService->Delete(m_PointShadowGPUBufferComp);

	g_Engine->Get<TextureResourceService>()->Delete(m_PointShadowAtlas);

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "LightDataService has been terminated.");
	return true;
}

bool LightDataService::Setup(IServiceConfig* systemConfig)
{
	m_Impl = new LightDataServiceImpl();

	return m_Impl->Setup(systemConfig);
}

bool LightDataService::Initialize()
{
	return m_Impl->Initialize();
}

bool LightDataService::Update()
{
	return m_Impl->Update();
}

bool LightDataService::Terminate()
{
	auto result = m_Impl->Terminate();
	delete m_Impl;
	return result;
}

ObjectStatus LightDataService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

GPUBufferComponent* LightDataService::GetPointLightBuffer()
{
	return m_Impl->m_PointLightGPUBufferComp;
}

GPUBufferComponent* LightDataService::GetSphereLightBuffer()
{
	return m_Impl->m_SphereLightGPUBufferComp;
}

GPUBufferComponent* LightDataService::GetCSMBuffer()
{
	return m_Impl->m_CSMGPUBufferComp;
}

GPUBufferComponent* LightDataService::GetGIBuffer()
{
	return m_Impl->m_GICBufferGPUBufferComp;
}

GPUBufferComponent* LightDataService::GetPointShadowBuffer()
{
	return m_Impl->m_PointShadowGPUBufferComp;
}

TextureComponent* LightDataService::GetPointShadowAtlas()
{
	return m_Impl->m_PointShadowAtlas;
}

uint32_t LightDataService::GetPointLightCount()
{
	return static_cast<uint32_t>(m_Impl->m_PointLightCBVector.size());
}

uint32_t LightDataService::GetSphereLightCount()
{
	return static_cast<uint32_t>(m_Impl->m_SphereLightCBVector.size());
}

uint32_t LightDataService::GetPointShadowCount()
{
	return static_cast<uint32_t>(m_Impl->m_PointShadowCBVector.size());
}

uint32_t LightDataService::GetPointLightAtlasSlot(uint32_t in_Index)
{
	return LookupAtlasSlot(m_Impl->m_PointLightAtlasSlot, in_Index, "GetPointLightAtlasSlot");
}

uint32_t LightDataService::GetSphereLightAtlasSlot(uint32_t in_Index)
{
	return LookupAtlasSlot(m_Impl->m_SphereLightAtlasSlot, in_Index, "GetSphereLightAtlasSlot");
}
