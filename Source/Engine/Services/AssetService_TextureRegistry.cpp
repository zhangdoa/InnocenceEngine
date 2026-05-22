#include "AssetService.h"
#include "AssetService_Internal.h"
#include "../Common/LogService.h"
#include "../Common/IOService.h"
#include "../Common/BCCompression.h"
#include "../ThirdParty/STBWrapper/STBWrapper.h"

#include "../Engine.h"
using namespace Inno;
using namespace AssetServiceNS;

TextureAssetHandle AssetService::AllocateTextureAsset(const char* name, ObjectLifespan lifespan)
{
	std::unique_lock<std::shared_mutex> l_lock(s_TextureMutex);

	auto l_existing = m_TextureLUT.find(name);
	if (l_existing != m_TextureLUT.end())
	{
		auto& l_asset = m_TextureAssets[l_existing->second.m_Index];
		if (l_asset.m_Residency != AssetResidency::Released)
			return l_existing->second;
	}

	uint32_t l_index;
	if (!m_TextureFreeSlots.empty())
	{
		l_index = m_TextureFreeSlots.back();
		m_TextureFreeSlots.pop_back();
		m_TextureAssets[l_index] = TextureAssetData();
	}
	else
	{
		l_index = static_cast<uint32_t>(m_TextureAssets.size());
		m_TextureAssets.emplace_back();
		m_TextureGenerations.emplace_back(0);
	}

	auto& l_asset = m_TextureAssets[l_index];
	l_asset.m_Lifespan = lifespan;
	l_asset.m_Residency = AssetResidency::Loading;
	l_asset.m_Name = name;

	TextureAssetHandle l_handle;
	l_handle.m_Index = l_index;
	l_handle.m_Generation = m_TextureGenerations[l_index];
	m_TextureLUT[name] = l_handle;

	return l_handle;
}

TextureAssetData* AssetService::GetTextureAsset(TextureAssetHandle handle)
{
	std::shared_lock<std::shared_mutex> l_lock(s_TextureMutex);

	if (!handle.IsValid() || handle.m_Index >= m_TextureAssets.size())
		return nullptr;

	if (handle.m_Generation != m_TextureGenerations[handle.m_Index])
		return nullptr;

	auto& l_asset = m_TextureAssets[handle.m_Index];
	if (l_asset.m_Residency == AssetResidency::Released)
		return nullptr;

	return &l_asset;
}

TextureAssetHandle AssetService::FindTextureAsset(const char* name)
{
	std::shared_lock<std::shared_mutex> l_lock(s_TextureMutex);
	auto l_result = m_TextureLUT.find(name);
	if (l_result != m_TextureLUT.end())
		return l_result->second;
	return TextureAssetHandle{};
}

std::string AssetService::ImportTexture(const char*          absolutePath,
                                        TextureSampler       sampler,
                                        TextureUsage         usage,
                                        bool                 isSRGB,
                                        uint32_t             slotIndex,
                                        const char*          instanceName,
                                        TextureChannelSource bc4Source)
{
	if (!absolutePath || !instanceName || !*instanceName)
	{
		Log(Error, "AssetService::ImportTexture: absolutePath and instanceName are required.");
		return {};
	}

	if (!g_Engine->Get<IOService>()->IsFileExist(absolutePath))
	{
		Log(Warning, "AssetService::ImportTexture: file not found: ", absolutePath);
		return {};
	}

	{
		std::lock_guard<std::mutex> l_lock(s_ImportTextureDedupMutex);
		if (!s_ImportTextureDedup.insert(instanceName).second)
		{
			Log(Verbose, "AssetService::ImportTexture: dedup skip ", instanceName);
			return std::string(instanceName);
		}
	}

	TextureComponent l_Texture = {};
	l_Texture.m_InstanceName        = instanceName;
	l_Texture.m_TextureDesc.Sampler = sampler;
	l_Texture.m_TextureDesc.Usage   = usage;
	l_Texture.m_TextureDesc.IsSRGB  = isSRGB;

	void* l_RawData = STBWrapper::Load(absolutePath, l_Texture);
	if (!l_RawData)
	{
		Log(Error, "AssetService::ImportTexture: STB decode failed: ", absolutePath);
		return {};
	}

	TextureDesc l_CompressedDesc = {};
	void* l_TextureData = BCCompression::CompressRGBAToBC(l_Texture.m_TextureDesc, l_RawData, slotIndex, bc4Source, l_CompressedDesc);
	if (!l_TextureData)
	{
		Log(Error, "AssetService::ImportTexture: BC compress failed: ", absolutePath);
		return {};
	}
	l_Texture.m_TextureDesc = l_CompressedDesc;

	if (!Save(l_Texture, l_TextureData))
	{
		Log(Error, "AssetService::ImportTexture: Save failed: ", absolutePath);
		return {};
	}

	Log(Success, "AssetService::ImportTexture: saved ", instanceName, " from ", absolutePath);
	return std::string(instanceName);
}
