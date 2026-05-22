#include "AssetService.h"
#include "AssetService_Internal.h"

using namespace Inno;
using namespace AssetServiceNS;

MeshAssetHandle AssetService::AllocateMeshAsset(const char* name, ObjectLifespan lifespan)
{
	std::unique_lock<std::shared_mutex> l_lock(s_MeshMutex);

	auto l_existing = m_MeshLUT.find(name);
	if (l_existing != m_MeshLUT.end())
	{
		auto& l_asset = m_MeshAssets[l_existing->second.m_Index];
		if (l_asset.m_Residency != AssetResidency::Released)
			return l_existing->second;
	}

	uint32_t l_index;
	if (!m_MeshFreeSlots.empty())
	{
		l_index = m_MeshFreeSlots.back();
		m_MeshFreeSlots.pop_back();
		m_MeshAssets[l_index] = MeshAsset();
	}
	else
	{
		l_index = static_cast<uint32_t>(m_MeshAssets.size());
		m_MeshAssets.emplace_back();
		m_MeshGenerations.emplace_back(0);
	}

	auto& l_asset = m_MeshAssets[l_index];
	l_asset.m_Lifespan = lifespan;
	l_asset.m_Residency = AssetResidency::Loading;
	l_asset.m_Name = name;

	MeshAssetHandle l_handle;
	l_handle.m_Index = l_index;
	l_handle.m_Generation = m_MeshGenerations[l_index];
	m_MeshLUT[name] = l_handle;

	return l_handle;
}

MeshAsset* AssetService::GetMeshAsset(MeshAssetHandle handle)
{
	std::shared_lock<std::shared_mutex> l_lock(s_MeshMutex);

	if (!handle.IsValid() || handle.m_Index >= m_MeshAssets.size())
		return nullptr;

	if (handle.m_Generation != m_MeshGenerations[handle.m_Index])
		return nullptr;

	auto& l_asset = m_MeshAssets[handle.m_Index];
	if (l_asset.m_Residency == AssetResidency::Released)
		return nullptr;

	return &l_asset;
}

uint32_t AssetService::DebugGetMeshGeneration(uint32_t index)
{
	std::shared_lock<std::shared_mutex> l_lock(s_MeshMutex);
	if (index >= m_MeshGenerations.size())
		return UINT32_MAX;
	return m_MeshGenerations[index];
}

MeshAssetHandle AssetService::FindMeshAsset(const char* name)
{
	std::shared_lock<std::shared_mutex> l_lock(s_MeshMutex);
	auto l_result = m_MeshLUT.find(name);
	if (l_result != m_MeshLUT.end())
		return l_result->second;
	return MeshAssetHandle{};
}
