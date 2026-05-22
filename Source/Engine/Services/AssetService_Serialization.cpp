#include "AssetService.h"
#include "../Common/Array.h"
#include "../Common/ComponentHeaders.h"
#include "../Common/LogService.h"
#include "../Common/IOService.h"
#include "../ThirdParty/JSONWrapper/JSONWrapper.h"
#include "../ThirdParty/STBWrapper/STBWrapper.h"

#include <filesystem>

#include "../Engine.h"
using namespace Inno;

bool AssetService::SaveScene(const char* fileName)
{
	return JSONWrapper::SaveScene(fileName);
}

bool AssetService::LoadScene(const char* fileName)
{
	return JSONWrapper::LoadScene(fileName);
}

bool AssetService::Load(const char* fileName, TransformComponent& component)
{
	return JSONWrapper::Load(fileName, component);
}

bool AssetService::Load(const char* fileName, MeshComponent& component, EntityID owner)
{
	return JSONWrapper::Load(fileName, component, owner);
}

bool AssetService::Load(const char* fileName, MaterialComponent& component, EntityID owner)
{
	return JSONWrapper::Load(fileName, component, owner);
}

bool AssetService::Load(const char* fileName, TextureComponent& component, EntityID owner)
{
	return JSONWrapper::Load(fileName, component, owner);
}

// bool AssetService::Load(const char* fileName, SkeletonComponent& component)
// {
//		return JSONWrapper::Load(fileName, component);
// }

// bool AssetService::Load(const char* fileName, AnimationComponent& component)
// {
//		return JSONWrapper::Load(fileName, component);
// }

bool AssetService::Load(const char* fileName, CameraComponent& component)
{
	return JSONWrapper::Load(fileName, component);
}

bool AssetService::Load(const char* fileName, LightComponent& component)
{
	return JSONWrapper::Load(fileName, component);
}

bool AssetService::Save(const char* fileName, const TextureDesc& textureDesc, void* textureData)
{
	return STBWrapper::Save(fileName, textureDesc, textureData);
}

bool AssetService::Save(const MeshComponent& component, Inno::Array<Vertex>& vertices, Inno::Array<Index>& indices)
{
	json j;
	JSONWrapper::to_json(j, component);

	// Add binary-specific fields
	j["VerticesNumber"] = vertices.size();
	j["IndicesNumber"] = indices.size();

	auto l_componentDir = GetComponentDirectory();
	std::filesystem::create_directories(l_componentDir);

	std::string l_baseName = component.m_InstanceName.c_str();
	auto l_binaryFileName = l_baseName + ".innobin";
	auto l_binaryFilePath = l_componentDir + l_binaryFileName;

	j["File"] = l_binaryFileName;

	std::ofstream l_binaryFile(l_binaryFilePath, std::ios::out | std::ios::trunc | std::ios::binary);
	if (!l_binaryFile.is_open())
	{
		Log(Error, "Failed to open binary mesh file: ", l_binaryFilePath.c_str());
		return false;
	}

	g_Engine->Get<IOService>()->SerializeVector(l_binaryFile, vertices);
	g_Engine->Get<IOService>()->SerializeVector(l_binaryFile, indices);
	l_binaryFile.close();

	auto filePath = GetAssetFilePath(component.m_InstanceName.c_str());
	return JSONWrapper::Save(filePath.c_str(), j);
}

bool AssetService::Save(const MaterialComponent& component)
{
	json j;
	JSONWrapper::to_json(j, component);
	auto filePath = GetAssetFilePath(component.m_InstanceName.c_str());
	return JSONWrapper::Save(filePath.c_str(), j);
}

bool AssetService::Save(const TextureComponent& component, void* textureData)
{
	json j;
	JSONWrapper::to_json(j, component);

	auto l_componentDir = GetComponentDirectory();
	std::filesystem::create_directories(l_componentDir);

	std::string l_baseName = component.m_InstanceName.c_str();
	auto l_binaryFileName = l_baseName + ".innobin";
	auto l_binaryFilePath = l_componentDir + l_binaryFileName;

	j["File"] = l_binaryFileName;

	bool binaryResult;
	if (component.m_TextureDesc.PixelDataType == TexturePixelDataType::Compressed)
	{
		uint32_t blockBytes;
		switch (component.m_TextureDesc.PixelDataFormat)
		{
		case TexturePixelDataFormat::BC1: blockBytes = 8;  break;
		case TexturePixelDataFormat::BC4: blockBytes = 8;  break;
		case TexturePixelDataFormat::BC3: blockBytes = 16; break;
		case TexturePixelDataFormat::BC5: blockBytes = 16; break;
		default:                          blockBytes = 0;  break;
		}
		if (blockBytes == 0)
		{
			Log(Error, "AssetService::Save: unknown BC format for texture: ", l_binaryFilePath.c_str());
			return false;
		}
		uint32_t blocksX = (component.m_TextureDesc.Width  + 3) / 4;
		uint32_t blocksY = (component.m_TextureDesc.Height + 3) / 4;
		size_t dataSize  = static_cast<size_t>(blocksX) * blocksY * blockBytes;
		auto* l_rawData = static_cast<const char*>(textureData);
		std::ofstream l_bcFile(l_binaryFilePath, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!l_bcFile.is_open())
		{
			Log(Error, "AssetService::Save: cannot open BC binary file: ", l_binaryFilePath.c_str());
			binaryResult = false;
		}
		else
		{
			l_bcFile.write(l_rawData, static_cast<std::streamsize>(dataSize));
			binaryResult = l_bcFile.good();
			l_bcFile.close();
		}
	}
	else
	{
		binaryResult = STBWrapper::Save(l_binaryFilePath.c_str(), component.m_TextureDesc, textureData);
	}
	if (!binaryResult)
	{
		Log(Error, "Failed to save texture binary data: ", l_binaryFilePath.c_str());
		return false;
	}

	auto filePath = GetAssetFilePath(component.m_InstanceName.c_str());
	return JSONWrapper::Save(filePath.c_str(), j);
}

bool AssetService::Save(const CameraComponent& component)
{
	json j;
	JSONWrapper::to_json(j, component);
	auto filePath = std::string();
	return JSONWrapper::Save(filePath.c_str(), j);
}

bool AssetService::Save(const LightComponent& component)
{
	json j;
	JSONWrapper::to_json(j, component);
	auto filePath = std::string();
	return JSONWrapper::Save(filePath.c_str(), j);
}
