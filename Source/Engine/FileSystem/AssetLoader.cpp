#include "AssetLoader.h"

#include "../../Engine/Core/InnoLogger.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

#include "../Core/IOService.h"
#include "JSONParser.h"
#include "TextureIO.h"

namespace InnoFileSystemNS::AssetLoader
{
	std::unordered_map<std::string, ModelMap> m_loadedModelMap;
	std::unordered_map<std::string, TextureDataComponent*> m_loadedTexture;

	ModelMap loadModelFromDisk(const std::string & fileName, bool AsyncUploadGPUResource = true);
}

ModelMap InnoFileSystemNS::AssetLoader::loadModel(const std::string & fileName, bool AsyncUploadGPUResource)
{
	auto l_extension = IOService::getFileExtension(fileName);
	if (l_extension == ".InnoModel")
	{
		ModelMap l_result;

		// check if this file has already been loaded once
		auto l_loadedModelMap = m_loadedModelMap.find(fileName);
		if (l_loadedModelMap != m_loadedModelMap.end())
		{
			InnoLogger::Log(LogLevel::Verbose, "FileSystem: AssetLoader: ", fileName.c_str(), " has been already loaded.");
			// Just copy new materials
			for (auto& i : l_loadedModelMap->second)
			{
				auto l_material = g_pModuleManager->getRenderingFrontend()->addMaterialDataComponent();
				*l_material = *i.second;
				l_result.emplace(i.first, l_material);
			}
			return l_result;
		}
		else
		{
			return loadModelFromDisk(fileName, AsyncUploadGPUResource);
		}
	}
	else
	{
		InnoLogger::Log(LogLevel::Warning, "FileSystem: AssimpWrapper: ", fileName.c_str(), " is not supported!");
		return ModelMap();
	}
}

ModelMap InnoFileSystemNS::AssetLoader::loadModelFromDisk(const std::string & fileName, bool AsyncUploadGPUResource)
{
	auto l_result = JSONParser::loadModelFromDisk(fileName, AsyncUploadGPUResource);
	m_loadedModelMap.emplace(fileName, l_result);

	return l_result;
}

TextureDataComponent* InnoFileSystemNS::AssetLoader::loadTexture(const std::string& fileName)
{
	TextureDataComponent* l_TDC;

	auto l_loadedTDC = m_loadedTexture.find(fileName);
	if (l_loadedTDC != m_loadedTexture.end())
	{
		InnoLogger::Log(LogLevel::Verbose, "FileSystem: AssetLoader: ", fileName.c_str(), " has been already loaded.");
		l_TDC = l_loadedTDC->second;
	}
	else
	{
		l_TDC = TextureIO::loadTexture(fileName);
		if (l_TDC)
		{
			m_loadedTexture.emplace(fileName, l_TDC);
		}
	}

	return l_TDC;
}