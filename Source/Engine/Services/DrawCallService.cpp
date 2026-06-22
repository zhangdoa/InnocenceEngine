#include "DrawCallService.h"
#include "DrawCallServiceImpl.h"

#include "../Common/LogService.h"
#include "../Common/Array.h"
#include "EntityRegistry.h"
#include "RenderingConfigurationService.h"
#include "AssetService.h"
#include "AssetService_Internal.h"
#include "../Component/MeshComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/WorldTransformComponent.h"
#include "../Component/VisibilityComponent.h"
#include "../Engine.h"
#include "GPUBufferResourceService.h"
#include "TextureResourceService.h"
#include "FrameManagementService.h"

using namespace Inno;

GPUBufferComponent* DrawCallServiceImpl::GetCurrentFrameTransformBuffer()
{
	auto l_frameCount = g_Engine->Get<FrameManagementService>()->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;
	return l_isOddFrame ? m_TransformBufferComp : m_TransformPrevBufferComp;
}

GPUBufferComponent* DrawCallServiceImpl::GetPreviousFrameTransformBuffer()
{
	auto l_frameCount = g_Engine->Get<FrameManagementService>()->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;
	return l_isOddFrame ? m_TransformPrevBufferComp : m_TransformBufferComp;
}

bool DrawCallServiceImpl::Setup(IServiceConfig* systemConfig)
{
	auto l_rsService = g_Engine->Get<GPUBufferResourceService>();

	m_RenderInstanceBufferComp = l_rsService->Add("RenderInstanceBuffer");
	m_TransformBufferComp = l_rsService->Add("TransformBuffer");
	m_TransformPrevBufferComp = l_rsService->Add("TransformPrevBuffer");
	m_MaterialGPUBufferComp = l_rsService->Add("MaterialCBuffer");
	m_MeshGeometryBufferComp = l_rsService->Add("MeshGeometryBuffer");

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool DrawCallServiceImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
	auto l_rsService = g_Engine->Get<GPUBufferResourceService>();

		auto l_RenderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();

		m_RenderInstanceBufferComp->m_GPUResourceType = GPUResourceType::Buffer;
		m_RenderInstanceBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
		m_RenderInstanceBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
		m_RenderInstanceBufferComp->m_ElementSize = sizeof(RenderInstance);

		l_rsService->Initialize(m_RenderInstanceBufferComp);

		m_TransformBufferComp->m_GPUResourceType = GPUResourceType::Buffer;
		m_TransformBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
		m_TransformBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
		m_TransformBufferComp->m_ElementSize = sizeof(TransformConstantBuffer);

		l_rsService->Initialize(m_TransformBufferComp);

		m_TransformPrevBufferComp->m_GPUResourceType = GPUResourceType::Buffer;
		m_TransformPrevBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
		m_TransformPrevBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
		m_TransformPrevBufferComp->m_ElementSize = sizeof(TransformConstantBuffer);

		l_rsService->Initialize(m_TransformPrevBufferComp);

		m_MaterialGPUBufferComp->m_GPUResourceType = GPUResourceType::Buffer;
		m_MaterialGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
		m_MaterialGPUBufferComp->m_ElementCount = l_RenderingCapability.maxMaterials;
		m_MaterialGPUBufferComp->m_ElementSize = sizeof(MaterialConstantBuffer);

		l_rsService->Initialize(m_MaterialGPUBufferComp);

		m_MeshGeometryBufferComp->m_GPUResourceType = GPUResourceType::Buffer;
		m_MeshGeometryBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
		m_MeshGeometryBufferComp->m_ElementCount = l_RenderingCapability.maxMeshes;
		m_MeshGeometryBufferComp->m_ElementSize = sizeof(MeshGeometry);

		l_rsService->Initialize(m_MeshGeometryBufferComp);

		m_ObjectStatus = ObjectStatus::Activated;
		Log(Success, "DrawCallService has been initialized.");
		return true;
	}
	else
	{
		Log(Error, "DrawCallService is not created!");
		return false;
	}
}

bool DrawCallServiceImpl::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		std::lock_guard<std::shared_mutex> l_lock(m_Mutex);

		// Rebuild the MeshGeometry table only on a resident-set change (staging locks
		// the asset mutex + scans every mesh asset — the real per-frame cost). The
		// upload below stays per-frame: the buffer is multi-buffered (one mapped copy
		// per swap-chain image) and Upload writes only the current slot, so a one-time
		// upload leaves 2 of 3 frames reading empty geometry (severe flicker).
		const uint64_t l_residencyEpoch = AssetService::GetMeshResidencyEpoch();
		if (l_residencyEpoch != m_LastStagedResidencyEpoch)
		{
			StageMeshGeometries();
			m_LastStagedResidencyEpoch = l_residencyEpoch;
		}
		CollectVisibleInstances();

		auto l_rsService = g_Engine->Get<GPUBufferResourceService>();

		// Cap each upload at the GPU buffer's ElementCount so a CPU-side overfill
		// cannot silently stomp past the buffer's end on the GPU. Log a warning at the
		// first overflow per frame so the miscount is visible.
		auto l_clamp = [](size_t produced, size_t capacity, const char* bufferName) -> size_t
		{
			if (produced <= capacity)
				return produced;
			Log(Warning, "DrawCallService: ", bufferName, " overflow — produced=", produced,
				" capacity=", capacity, " — dropping ", (produced - capacity), " entries.");
			return capacity;
		};

	if (m_RenderInstanceVector.size() > 0)
	{
		auto l_n = l_clamp(m_RenderInstanceVector.size(), m_RenderInstanceBufferComp->m_ElementCount, "RenderInstanceBuffer");
		l_rsService->Upload(m_RenderInstanceBufferComp, m_RenderInstanceVector, 0, l_n);
	}
	if (m_TransformBufferVector.size() > 0)
	{
		auto l_currentFrameTransformBuffer = GetCurrentFrameTransformBuffer();
		auto l_n = l_clamp(m_TransformBufferVector.size(), l_currentFrameTransformBuffer->m_ElementCount, "TransformBuffer");
		l_rsService->Upload(l_currentFrameTransformBuffer, m_TransformBufferVector, 0, l_n);
	}
	if (m_MaterialCBVector.size() > 0)
	{
		auto l_n = l_clamp(m_MaterialCBVector.size(), m_MaterialGPUBufferComp->m_ElementCount, "MaterialCBuffer");
		l_rsService->Upload(m_MaterialGPUBufferComp, m_MaterialCBVector, 0, l_n);
	}
	if (m_MeshGeometryVector.size() > 0)
	{
		auto l_n = l_clamp(m_MeshGeometryVector.size(), m_MeshGeometryBufferComp->m_ElementCount, "MeshGeometryBuffer");
		l_rsService->Upload(m_MeshGeometryBufferComp, m_MeshGeometryVector, 0, l_n);
	}
		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool DrawCallServiceImpl::Terminate()
{
	auto l_rsService = g_Engine->Get<GPUBufferResourceService>();

	l_rsService->Delete(m_RenderInstanceBufferComp);
	l_rsService->Delete(m_TransformBufferComp);
	l_rsService->Delete(m_TransformPrevBufferComp);
	l_rsService->Delete(m_MaterialGPUBufferComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "DrawCallService has been terminated.");
	return true;
}

bool DrawCallService::Setup(IServiceConfig* systemConfig)
{
	m_Impl = new DrawCallServiceImpl();

	return m_Impl->Setup(systemConfig);
}

bool DrawCallService::Initialize()
{
	return m_Impl->Initialize();
}

bool DrawCallService::Update()
{
	return m_Impl->Update();
}

bool DrawCallService::Terminate()
{
	auto result = m_Impl->Terminate();
	delete m_Impl;
	return result;
}

ObjectStatus DrawCallService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

const Inno::Array<RenderInstance>& DrawCallService::GetRenderInstances()
{
	std::lock_guard<std::shared_mutex> l_lock(m_Impl->m_Mutex);
	return m_Impl->m_RenderInstanceVector;
}

GPUBufferComponent* DrawCallService::GetRenderInstanceBuffer()
{
	return m_Impl->m_RenderInstanceBufferComp;
}

GPUBufferComponent* DrawCallService::GetCurrentFrameTransformBuffer()
{
	return m_Impl->GetCurrentFrameTransformBuffer();
}

GPUBufferComponent* DrawCallService::GetPreviousFrameTransformBuffer()
{
	return m_Impl->GetPreviousFrameTransformBuffer();
}

GPUBufferComponent* DrawCallService::GetMaterialBuffer()
{
	return m_Impl->m_MaterialGPUBufferComp;
}
