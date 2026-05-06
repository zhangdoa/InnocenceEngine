#include "AssetService.h"
#include "AssetService_Internal.h"

using namespace Inno;
using namespace AssetServiceNS;

AssetService::MaterialAssetAllocation AssetService::AllocateMaterialAsset(const char* name, ObjectLifespan lifespan)
{
	std::unique_lock<std::shared_mutex> l_lock(s_MaterialMutex);

	auto l_existing = m_MaterialLUT.find(name);
	if (l_existing != m_MaterialLUT.end())
	{
		auto& l_asset = m_MaterialAssets[l_existing->second.m_Index];
		if (l_asset.m_Residency != AssetResidency::Released)
			return { l_existing->second, false };
	}

	uint32_t l_index;
	if (!m_MaterialFreeSlots.empty())
	{
		l_index = m_MaterialFreeSlots.back();
		m_MaterialFreeSlots.pop_back();
		m_MaterialAssets[l_index] = MaterialAssetData();
	}
	else
	{
		l_index = static_cast<uint32_t>(m_MaterialAssets.size());
		m_MaterialAssets.emplace_back();
		m_MaterialGenerations.emplace_back(0);
	}

	auto& l_asset = m_MaterialAssets[l_index];
	l_asset.m_Lifespan = lifespan;
	l_asset.m_Residency = AssetResidency::Loading;
	l_asset.m_Name = name;

	MaterialAssetHandle l_handle;
	l_handle.m_Index = l_index;
	l_handle.m_Generation = m_MaterialGenerations[l_index];
	m_MaterialLUT[name] = l_handle;

	return { l_handle, true };
}

MaterialAssetData* AssetService::GetMaterialAsset(MaterialAssetHandle handle)
{
	std::shared_lock<std::shared_mutex> l_lock(s_MaterialMutex);

	if (!handle.IsValid() || handle.m_Index >= m_MaterialAssets.size())
		return nullptr;

	if (handle.m_Generation != m_MaterialGenerations[handle.m_Index])
		return nullptr;

	auto& l_asset = m_MaterialAssets[handle.m_Index];
	if (l_asset.m_Residency == AssetResidency::Released)
		return nullptr;

	return &l_asset;
}

MaterialAssetHandle AssetService::FindMaterialAsset(const char* name)
{
	std::shared_lock<std::shared_mutex> l_lock(s_MaterialMutex);
	auto l_result = m_MaterialLUT.find(name);
	if (l_result != m_MaterialLUT.end())
		return l_result->second;
	return MaterialAssetHandle{};
}
