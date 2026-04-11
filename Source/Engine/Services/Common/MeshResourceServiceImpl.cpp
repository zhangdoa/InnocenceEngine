#include "../MeshResourceService.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/MathHelper.h"
#include "../../Engine.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/EntityRegistry.h"
#include "../../Services/AssetService.h"

using namespace Inno;

bool MeshResourceService::Setup(IServiceConfig* systemConfig)
{
	auto l_cap = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();
	m_Pool.Initialize(l_cap.maxMeshes);

	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "MeshResourceService Setup finished.");
	return true;
}

bool MeshResourceService::Terminate()
{
	m_Pool.Terminate();
	m_MeshResources.clear();
	m_FreeMeshResourceSlots.clear();
	m_MeshResourceLUT.clear();

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "MeshResourceService Terminated.");
	return true;
}

MeshComponent* MeshResourceService::Add(const char* name)
{
	return m_Pool.Allocate(name);
}

bool MeshResourceService::Delete(MeshComponent* ptr)
{
	m_Pool.Release(ptr);
	return true;
}

void MeshResourceService::Initialize(MeshComponent* mesh, std::vector<Vertex>& vertices, std::vector<Index>& indices, EntityID owner)
{
	if (mesh->m_ObjectStatus == ObjectStatus::Activated)
		return;

	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto l_lifespan = (owner != INVALID_ENTITY) ? l_registry->GetLifespan(owner) : ObjectLifespan::Persistence;

	auto l_assetHandle = AssetService::AllocateMeshAsset(mesh->m_InstanceName.c_str(), l_lifespan);
	if (!l_assetHandle.IsValid())
	{
		Log(Error, "Failed to allocate MeshAsset for: ", mesh->m_InstanceName);
		return;
	}

	mesh->m_Asset = l_assetHandle;

	AllocateMeshResource(mesh->m_InstanceName.c_str(), l_lifespan);

	auto* l_resource = AssetService::GetMeshAsset(l_assetHandle);
	if (!vertices.empty())
	{
		l_resource->m_AABB = Math::GenerateAABB(vertices.data(), vertices.size());
		Log(Verbose, "Calculated AABB for MeshComponent: min(",
			l_resource->m_AABB.m_boundMin.x, ",", l_resource->m_AABB.m_boundMin.y, ",", l_resource->m_AABB.m_boundMin.z,
			") max(", l_resource->m_AABB.m_boundMax.x, ",", l_resource->m_AABB.m_boundMax.y, ",", l_resource->m_AABB.m_boundMax.z, ")");
	}

	m_DeferredQueue.push(MeshInitTask(mesh, std::move(vertices), std::move(indices), owner, l_lifespan));
	Log(Verbose, "MeshComponent ", mesh->m_InstanceName, " queued for deferred initialization");
}

bool MeshResourceService::InitializeComponents()
{
	while (m_DeferredQueue.size() > 0)
	{
		MeshInitTask l_task(nullptr, std::vector<Vertex>(), std::vector<Index>());
		m_DeferredQueue.tryPop(l_task);

		if (!l_task.m_Component)
			continue;

		MeshComponent* l_meshComp = l_task.m_Component;
		if (l_task.m_Owner != INVALID_ENTITY)
		{
			MeshComponent* l_current = g_Engine->Get<EntityRegistry>()->Get<MeshComponent>(l_task.m_Owner);
			if (l_current)
				l_meshComp = l_current;
			else
				Log(Warning, "MeshInitTask: entity ", l_task.m_Owner, " no longer has MeshComponent, using stored pointer");
		}

		auto* l_resource = AssetService::GetMeshAsset(l_meshComp->m_Asset);
		if (!l_resource)
		{
			Log(Error, "MeshInitTask: invalid MeshAssetHandle for ", l_meshComp->m_InstanceName);
			continue;
		}

		// If the asset is already resident (built by a previous task sharing this handle),
		// just activate the component without re-running GPU init — re-initializing a mapped
		// upload buffer while it is still in use causes heap corruption.
		if (l_resource->m_Residency == AssetResidency::Resident)
		{
			l_meshComp->m_ObjectStatus = ObjectStatus::Activated;
			continue;
		}

		// Tasks with no vertex data are activation-only: they share a handle with another mesh
		// whose primary init task has not yet been processed. Re-queue and wait.
		if (l_task.m_Vertices.empty())
		{
			m_DeferredQueue.push(std::move(l_task));
			continue;
		}

		// Save before InitializeImpl: the GPU wait inside may stall for ms, during which
		// the main thread emplaces more components or assets — reallocation of those
		// std::vectors would make l_meshComp and l_resource dangling.
		auto l_assetHandle = l_meshComp->m_Asset;
		Log(Verbose, "Processing deferred mesh initialization for: ", l_meshComp->m_InstanceName);
		if (InitializeImpl(l_assetHandle, l_task.m_Vertices, l_task.m_Indices))
		{
			// Re-fetch pointers: std::vector backing may have been reallocated.
			auto* l_freshResource = AssetService::GetMeshAsset(l_assetHandle);
			if (l_freshResource)
				l_freshResource->m_Residency = AssetResidency::Resident;

			MeshComponent* l_activateTarget = l_task.m_Component;
			if (l_task.m_Owner != INVALID_ENTITY)
			{
				MeshComponent* l_current = g_Engine->Get<EntityRegistry>()->Get<MeshComponent>(l_task.m_Owner);
				if (l_current)
					l_activateTarget = l_current;
			}
			l_activateTarget->m_ObjectStatus = ObjectStatus::Activated;
		}
		else
			m_DeferredQueue.push(std::move(l_task));
	}

	return true;
}

bool MeshResourceService::OnSceneUnloading()
{
	ReleaseAllMeshResources(ObjectLifespan::Scene);

	auto l_registry = g_Engine->Get<EntityRegistry>();
	auto l_isSceneBound = [&](EntityID owner) {
		return owner != INVALID_ENTITY
			&& l_registry->GetLifespan(owner) == ObjectLifespan::Scene;
	};

	std::vector<MeshInitTask> l_persistent;
	MeshInitTask l_task(nullptr, {}, {});
	while (m_DeferredQueue.tryPop(l_task))
	{
		if (!l_isSceneBound(l_task.m_Owner))
			l_persistent.push_back(std::move(l_task));
	}
	for (auto& t : l_persistent)
		m_DeferredQueue.push(std::move(t));

	return true;
}

GPUMeshResourceHandle MeshResourceService::AllocateMeshResource(const char* name, ObjectLifespan lifespan)
{
	auto l_existing = m_MeshResourceLUT.find(name);
	if (l_existing != m_MeshResourceLUT.end())
		return l_existing->second;

	uint32_t l_index;
	if (!m_FreeMeshResourceSlots.empty())
	{
		l_index = m_FreeMeshResourceSlots.back();
		m_FreeMeshResourceSlots.pop_back();
		m_MeshResources[l_index] = GPUMeshResource();
	}
	else
	{
		l_index = static_cast<uint32_t>(m_MeshResources.size());
		m_MeshResources.emplace_back();
	}

	auto& l_resource = m_MeshResources[l_index];
	l_resource.m_Lifespan = lifespan;
	l_resource.m_Status = ObjectStatus::Created;
	l_resource.m_Name = name;

	GPUMeshResourceHandle l_handle;
	l_handle.m_Index = l_index;
	m_MeshResourceLUT.emplace(name, l_handle);

	return l_handle;
}

void MeshResourceService::ReleaseMeshResource(GPUMeshResourceHandle handle)
{
	if (!handle.IsValid() || handle.m_Index >= m_MeshResources.size())
		return;

	auto& l_resource = m_MeshResources[handle.m_Index];
	if (l_resource.m_Status == ObjectStatus::Invalid)
		return;

	MeshAssetHandle l_assetHandle;
	l_assetHandle.m_Index = handle.m_Index;
	ReleaseMeshGPUResourceImpl(l_assetHandle);

	m_MeshResourceLUT.erase(std::string(l_resource.m_Name.c_str()));
	l_resource = GPUMeshResource();
	m_FreeMeshResourceSlots.push_back(handle.m_Index);
}

void MeshResourceService::ReleaseAllMeshResources(ObjectLifespan lifespan)
{
	for (uint32_t i = 0; i < static_cast<uint32_t>(m_MeshResources.size()); i++)
	{
		if (m_MeshResources[i].m_Lifespan == lifespan && m_MeshResources[i].m_Status != ObjectStatus::Invalid)
		{
			GPUMeshResourceHandle l_handle;
			l_handle.m_Index = i;
			ReleaseMeshResource(l_handle);
		}
	}
}

GPUMeshResource* MeshResourceService::GetMeshResource(GPUMeshResourceHandle handle)
{
	if (!handle.IsValid() || handle.m_Index >= m_MeshResources.size())
		return nullptr;

	auto& l_resource = m_MeshResources[handle.m_Index];
	if (l_resource.m_Status == ObjectStatus::Invalid)
		return nullptr;

	return &l_resource;
}

const GPUMeshResource* MeshResourceService::GetMeshResource(GPUMeshResourceHandle handle) const
{
	if (!handle.IsValid() || handle.m_Index >= m_MeshResources.size())
		return nullptr;

	auto& l_resource = m_MeshResources[handle.m_Index];
	if (l_resource.m_Status == ObjectStatus::Invalid)
		return nullptr;

	return &l_resource;
}

GPUMeshResourceHandle MeshResourceService::FindMeshResourceByName(const char* name)
{
	auto l_result = m_MeshResourceLUT.find(name);
	if (l_result != m_MeshResourceLUT.end())
		return l_result->second;
	return INVALID_GPU_MESH_HANDLE;
}
