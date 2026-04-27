#include "LightDataService.h"

#include "../Common/LogService.h"
#include "../Common/MathHelper.h"
#include "../Common/GPUDataStructure.h"
#include "EntityRegistry.h"
#include "RenderingConfigurationService.h"
#include "../Component/LightComponent.h"
#include "../Component/TransformComponent.h"
#include "../Engine.h"
#include "GPUBufferResourceService.h"
#include "TextureResourceService.h"
#include "../Component/TextureComponent.h"

using namespace Inno;

namespace
{
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
		std::vector<PointShadowConstantBuffer> m_PointShadowCBVector;

		// Parallel-indexed to m_PointLightCBVector / m_SphereLightCBVector;
		// initialized to INVALID_ATLAS_SLOT in UpdateLightData, overwritten
		// by UpdatePointShadowData for shadow-casting lights.
		std::vector<uint32_t> m_PointLightAtlasSlot;
		std::vector<uint32_t> m_SphereLightAtlasSlot;

		GPUBufferComponent* m_PointLightGPUBufferComp   = nullptr;
		GPUBufferComponent* m_SphereLightGPUBufferComp  = nullptr;
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

		m_GICBufferGPUBufferComp->m_ElementSize = sizeof(GIConstantBuffer);
		m_GICBufferGPUBufferComp->m_ElementCount = 1;

		l_rsService->Initialize(m_GICBufferGPUBufferComp);

		m_PointShadowGPUBufferComp->m_ElementCount = l_RenderingCapability.maxPointShadows;
		m_PointShadowGPUBufferComp->m_ElementSize = sizeof(PointShadowConstantBuffer);

		l_rsService->Initialize(m_PointShadowGPUBufferComp);

		// Atlas resource: Texture2DArray, R32G32_FLOAT, DepthOrArraySize =
		// maxPointShadows * 6. Per design call (TASK-150) the caster (TASK-148)
		// writes packed-depth `(linearDist, linearDist², 0, 1)` and the resolver
		// only reads `.r` (depth) and `.g` (depth²). RGBA32F at 256² × 48 slices
		// × triple-buffer = 144 MB; RG32F is exactly half that = 72 MB. The
		// surfacing producer's "12 MB" estimate was 4 B/pixel arithmetic that
		// did not account for color-RT format width. Saving 72 MB of VRAM is
		// the primary gain; rendering output is unchanged because the unread
		// .b/.a channels were write-only padding.
		// IsMultiBuffer=true matches sun-shadow precedent for swap-chain
		// cycling; ColorAttachment usage routes through the RT bind-flag path
		// (DX12Helper::GetTextureBindFlags).
		const uint32_t l_PerFaceResolution = 256;
		const uint32_t l_AtlasSliceCount   = l_RenderingCapability.maxPointShadows * 6;
		m_PointShadowAtlas->m_TextureDesc.Sampler           = TextureSampler::Sampler2DArray;
		m_PointShadowAtlas->m_TextureDesc.Usage             = TextureUsage::ColorAttachment;
		m_PointShadowAtlas->m_TextureDesc.IsMultiBuffer     = true;
		m_PointShadowAtlas->m_TextureDesc.PixelDataFormat   = TexturePixelDataFormat::RG;
		m_PointShadowAtlas->m_TextureDesc.PixelDataType     = TexturePixelDataType::Float32;
		m_PointShadowAtlas->m_TextureDesc.Width             = l_PerFaceResolution;
		m_PointShadowAtlas->m_TextureDesc.Height            = l_PerFaceResolution;
		m_PointShadowAtlas->m_TextureDesc.DepthOrArraySize  = l_AtlasSliceCount;
		m_PointShadowAtlas->m_TextureDesc.MipLevels         = 1;
		// Clear/border to far plane (linearDist=1) so unrendered texels don't
		// register as blockers at depth 0 in PCSS. Only .r and .g matter —
		// caster writes the same shape and resolver only reads the same shape.
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

		// pos.w carries the atlas slot index (TASK-148): seeded to a sentinel
		// (INVALID_ATLAS_SLOT bit-cast to float), overwritten by
		// UpdatePointShadowData() for shadow-casting lights before upload. The
		// shader recovers via asuint(pos.w) — sentinel matches the
		// INVALID_TEXTURE_INDEX idiom on Material_CB. We use memcpy rather than
		// std::bit_cast because the project is C++17.
		float l_SentinelSlot;
		const uint32_t l_SentinelBits = INVALID_ATLAS_SLOT;
		std::memcpy(&l_SentinelSlot, &l_SentinelBits, sizeof(l_SentinelSlot));

		if (l_Light.m_LightType == LightType::Point)
		{
			PointLightConstantBuffer l_data;
			if (l_Transform)
				l_data.pos = l_Transform->m_LocalPos;
			l_data.luminance = l_Light.m_RGBColor * l_Light.m_LuminousFlux;
			l_data.luminance.w = l_Light.m_Shape.x;
			l_data.pos.w = l_SentinelSlot;
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
			l_data.pos.w = l_SentinelSlot;
			m_SphereLightCBVector.emplace_back(l_data);
			m_SphereLightAtlasSlot.emplace_back(INVALID_ATLAS_SLOT);
		}
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
