#include "AssetService.h"
#include "AssetService_Internal.h"
#include "../Common/LogService.h"

#include "../Engine.h"
using namespace Inno;

namespace Inno::AssetServiceNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	std::deque<MeshAsset> m_MeshAssets;
	std::vector<uint32_t> m_MeshFreeSlots;
	std::vector<uint32_t> m_MeshGenerations;
	std::unordered_map<std::string, MeshAssetHandle> m_MeshLUT;
	std::shared_mutex s_MeshMutex;

	std::deque<MaterialAsset> m_MaterialAssets;
	std::vector<uint32_t> m_MaterialFreeSlots;
	std::vector<uint32_t> m_MaterialGenerations;
	std::unordered_map<std::string, MaterialAssetHandle> m_MaterialLUT;
	std::shared_mutex s_MaterialMutex;

	std::deque<TextureAsset> m_TextureAssets;
	std::vector<uint32_t> m_TextureFreeSlots;
	std::vector<uint32_t> m_TextureGenerations;
	std::unordered_map<std::string, TextureAssetHandle> m_TextureLUT;
	std::shared_mutex s_TextureMutex;

	std::mutex s_ImportTextureDedupMutex;
	std::unordered_set<std::string> s_ImportTextureDedup;
}

using namespace AssetServiceNS;

bool AssetService::Setup(IServiceConfig* systemConfig)
{
	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool AssetService::Initialize()
{
	return true;
}

bool AssetService::Update()
{
	return true;
}

bool AssetService::Terminate()
{
	Log(Success, "AssetService: clearing asset registries...");
	// Clear all asset registries before CRT static cleanup.
	// MaterialAsset contains std::vector<std::string> which must be freed while the
	// heap is still valid — deferring to the static-destructor phase can cause
	// STATUS_HEAP_CORRUPTION on exit if the deques are non-empty.
	std::unique_lock<std::shared_mutex> l_meshLock(s_MeshMutex);
	m_MeshAssets.clear();
	m_MeshFreeSlots.clear();
	m_MeshGenerations.clear();
	m_MeshLUT.clear();
	l_meshLock.unlock();

	std::unique_lock<std::shared_mutex> l_matLock(s_MaterialMutex);
	m_MaterialAssets.clear();
	m_MaterialFreeSlots.clear();
	m_MaterialGenerations.clear();
	m_MaterialLUT.clear();
	l_matLock.unlock();

	std::unique_lock<std::shared_mutex> l_texLock(s_TextureMutex);
	m_TextureAssets.clear();
	m_TextureFreeSlots.clear();
	m_TextureGenerations.clear();
	m_TextureLUT.clear();
	l_texLock.unlock();

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "AssetService has been terminated.");
	return true;
}

ObjectStatus AssetService::GetStatus()
{
	return ObjectStatus();
}

void AssetService::ReleaseAssetsByLifespan(ObjectLifespan lifespan)
{
	uint32_t l_meshReleased = 0, l_matReleased = 0, l_texReleased = 0;
	{
		std::unique_lock<std::shared_mutex> l_lock(s_MeshMutex);
		for (uint32_t i = 0; i < static_cast<uint32_t>(m_MeshAssets.size()); i++)
		{
			auto& l_asset = m_MeshAssets[i];
			if (l_asset.m_Lifespan == lifespan && l_asset.m_Residency != AssetResidency::Released)
			{
				m_MeshLUT.erase(std::string(l_asset.m_Name.c_str()));
				l_asset = MeshAsset();
				l_asset.m_Residency = AssetResidency::Released;
				m_MeshGenerations[i]++;
				m_MeshFreeSlots.push_back(i);
				++l_meshReleased;
			}
		}
	}

	{
		std::unique_lock<std::shared_mutex> l_lock(s_MaterialMutex);
		for (uint32_t i = 0; i < static_cast<uint32_t>(m_MaterialAssets.size()); i++)
		{
			auto& l_asset = m_MaterialAssets[i];
			if (l_asset.m_Lifespan == lifespan && l_asset.m_Residency != AssetResidency::Released)
			{
				m_MaterialLUT.erase(std::string(l_asset.m_Name.c_str()));
				l_asset = MaterialAsset();
				l_asset.m_Residency = AssetResidency::Released;
				m_MaterialGenerations[i]++;
				m_MaterialFreeSlots.push_back(i);
				++l_matReleased;
			}
		}
	}

	{
		std::unique_lock<std::shared_mutex> l_lock(s_TextureMutex);
		for (uint32_t i = 0; i < static_cast<uint32_t>(m_TextureAssets.size()); i++)
		{
			auto& l_asset = m_TextureAssets[i];
			if (l_asset.m_Lifespan == lifespan && l_asset.m_Residency != AssetResidency::Released)
			{
				m_TextureLUT.erase(std::string(l_asset.m_Name.c_str()));
				l_asset = TextureAsset();
				l_asset.m_Residency = AssetResidency::Released;
				m_TextureGenerations[i]++;
				m_TextureFreeSlots.push_back(i);
				++l_texReleased;
			}
		}
	}

	Log(Verbose, "AssetService::ReleaseAssetsByLifespan(", lifespan,
		") — released meshes=", l_meshReleased, " materials=", l_matReleased, " textures=", l_texReleased);
}
