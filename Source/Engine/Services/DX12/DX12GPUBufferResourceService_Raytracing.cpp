#include "DX12GPUBufferResourceService.h"
#include "DX12MeshResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"
#include "../FrameManagementService.h"
#include "../MeshResourceService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Engine.h"
#include "../../Services/EntityRegistry.h"
#include "../../Component/WorldTransformComponent.h"
#include "../../Component/MeshComponent.h"
#include "../../Common/MathHelper.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12GPUBufferResourceService::OnSceneLoadingStart()
{
	for (size_t i = 0; i < m_RaytracingInstanceDescs.size(); i++)
	{
		auto l_descList = reinterpret_cast<DX12RaytracingInstanceDescList*>(m_RaytracingInstanceDescs[i]);
		l_descList->m_Descs.clear();
	}

	m_TLASReady = false;
	m_PrevInstanceCount = 0;

	Log(Verbose, "Raytracing instance descriptions have been cleared.");

	return true;
}

bool DX12GPUBufferResourceService::UpdateRaytracingInstances()
{
	if (m_RaytracingInstanceDescs.empty())
		return true;

	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto& l_meshStorage = l_registry->Storage<MeshComponent>();
	const auto& l_meshOwners = l_meshStorage.AllOwners();

	if (l_meshOwners.empty())
		return true;

	auto l_meshService = static_cast<DX12MeshResourceService*>(g_Engine->Get<MeshResourceService>());

	bool l_needRebuild = false;

	if (l_meshOwners.size() != m_PrevInstanceCount)
		l_needRebuild = true;

	if (!l_needRebuild)
	{
		for (EntityID l_entity : l_meshOwners)
		{
			auto* l_world = l_registry->Get<WorldTransformComponent>(l_entity);
			if (l_world && l_world->m_Dirty)
			{
				l_needRebuild = true;
				break;
			}
		}
	}

	if (!l_needRebuild)
		return true;

	{
		size_t l_dirtyCount = 0;
		for (EntityID l_entity : l_meshOwners)
		{
			auto* l_world = l_registry->Get<WorldTransformComponent>(l_entity);
			if (l_world && l_world->m_Dirty)
				++l_dirtyCount;
		}
		Log(Verbose, "TLAS rebuild: frame=",
			g_Engine->Get<FrameManagementService>()->GetFrameCountSinceLaunch(),
			" instances=", l_meshOwners.size(), " prevCount=", m_PrevInstanceCount,
			" dirtyTransforms=", l_dirtyCount);
	}

	auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();

	for (size_t frameIndex = 0; frameIndex < l_swapChainImageCount; frameIndex++)
	{
		auto l_descList = reinterpret_cast<DX12RaytracingInstanceDescList*>(m_RaytracingInstanceDescs[frameIndex]);
		l_descList->m_Descs.clear();

		for (EntityID l_entity : l_meshOwners)
		{
			auto* l_mesh = l_registry->Get<MeshComponent>(l_entity);
			if (!l_mesh || !l_mesh->m_Asset.IsValid())
				continue;

			if (l_mesh->m_ObjectStatus != ObjectStatus::Activated)
				continue;

			uint64_t l_blasAddress = l_meshService->GetBLASAddress(l_mesh->m_Asset);
			uint32_t l_vertexSRVSlot = l_meshService->GetVertexSRVSlot(l_mesh->m_Asset);
			if (l_blasAddress == 0 || l_vertexSRVSlot == UINT32_MAX) continue;

			auto* l_world = l_registry->Get<WorldTransformComponent>(l_entity);
			Mat4 l_transform = l_world ? l_world->m_WorldMatrix : Math::generateIdentityMatrix<float>();

			D3D12_RAYTRACING_INSTANCE_DESC l_instanceDesc = {};

			l_instanceDesc.Transform[0][0] = l_transform.m00;
			l_instanceDesc.Transform[0][1] = l_transform.m01;
			l_instanceDesc.Transform[0][2] = l_transform.m02;
			l_instanceDesc.Transform[0][3] = l_transform.m03;

			l_instanceDesc.Transform[1][0] = l_transform.m10;
			l_instanceDesc.Transform[1][1] = l_transform.m11;
			l_instanceDesc.Transform[1][2] = l_transform.m12;
			l_instanceDesc.Transform[1][3] = l_transform.m13;

			l_instanceDesc.Transform[2][0] = l_transform.m20;
			l_instanceDesc.Transform[2][1] = l_transform.m21;
			l_instanceDesc.Transform[2][2] = l_transform.m22;
			l_instanceDesc.Transform[2][3] = l_transform.m23;

			l_instanceDesc.InstanceID = l_vertexSRVSlot;
			l_instanceDesc.InstanceMask = 0xFF;
			l_instanceDesc.InstanceContributionToHitGroupIndex = 0;
			l_instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			l_instanceDesc.AccelerationStructure = l_blasAddress;

			l_descList->m_Descs.emplace_back(l_instanceDesc);
		}
	}

	m_PrevInstanceCount = l_meshOwners.size();
	m_TLASReady = false;

	return true;
}

bool DX12GPUBufferResourceService::CreateRaytracingResources()
{
	auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();

	m_TLASBufferComponent = Add("TLASBuffer");
	m_TLASBufferComponent->m_GPUAccessibility = Accessibility::ReadWrite;
	m_TLASBufferComponent->m_Usage = GPUBufferUsage::TLAS;
	m_TLASBufferComponent->m_ElementCount = 256;

	InitializeImpl(m_TLASBufferComponent);

	m_ScratchBufferComponent = Add("ScratchBuffer");
	m_ScratchBufferComponent->m_GPUAccessibility = Accessibility::ReadWrite;
	m_ScratchBufferComponent->m_Usage = GPUBufferUsage::ScratchBuffer;
	m_ScratchBufferComponent->m_ElementCount = 256;

	InitializeImpl(m_ScratchBufferComponent);

	m_RaytracingInstanceBufferComponent = Add("RaytracingInstanceBuffer");
	m_RaytracingInstanceBufferComponent->m_GPUAccessibility = Accessibility::ReadWrite;
	m_RaytracingInstanceBufferComponent->m_ElementCount = 256;
	m_RaytracingInstanceBufferComponent->m_ElementSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

	InitializeImpl(m_RaytracingInstanceBufferComponent);

	m_RaytracingInstanceDescs.resize(l_swapChainImageCount);
	for (size_t i = 0; i < m_RaytracingInstanceDescs.size(); i++)
	{
		auto l_descList = new DX12RaytracingInstanceDescList();
		l_descList->m_Descs.reserve(m_RaytracingInstanceBufferComponent->m_ElementCount);
		m_RaytracingInstanceDescs[i] = l_descList;
	}

	return true;
}

bool DX12GPUBufferResourceService::ReleaseRaytracingResources()
{
	Delete(m_RaytracingInstanceBufferComponent);
	Delete(m_ScratchBufferComponent);
	Delete(m_TLASBufferComponent);

	return true;
}
