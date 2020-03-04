#include "AssetLoader.h"

#include "../../Engine/Core/InnoLogger.h"

#include "../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

#include "../Core/IOService.h"
#include "JSONParser.h"
#include "TextureIO.h"

namespace InnoFileSystemNS::AssetLoader
{
	ModelIndex loadModelFromDisk(const char* fileName, bool AsyncUploadGPUResource = true);
}

ModelIndex InnoFileSystemNS::AssetLoader::loadModel(const char* fileName, bool AsyncUploadGPUResource)
{
	auto l_extension = IOService::getFileExtension(fileName);
	if (l_extension == ".InnoModel")
	{
		ModelIndex l_loadedModelIndex;

		// check if this file has already been loaded once
		if (g_pModuleManager->getAssetSystem()->findLoadedModel(fileName, l_loadedModelIndex))
		{
			// Just copy new materials
			ModelIndex l_result;

			l_result.m_startOffset = g_pModuleManager->getAssetSystem()->getCurrentMeshMaterialPairOffset();

			for (uint64_t i = 0; i < l_loadedModelIndex.m_count; i++)
			{
				auto l_newPair = g_pModuleManager->getAssetSystem()->getMeshMaterialPair(l_loadedModelIndex.m_startOffset + i);
				l_newPair.material = g_pModuleManager->getRenderingFrontend()->addMaterialDataComponent();
				g_pModuleManager->getAssetSystem()->addMeshMaterialPair(l_newPair);
			}

			l_result.m_count = l_loadedModelIndex.m_count;

			return l_result;
		}
		else
		{
			return loadModelFromDisk(fileName, AsyncUploadGPUResource);
		}
	}
	else
	{
		InnoLogger::Log(LogLevel::Warning, "FileSystem: AssimpWrapper: ", fileName, " is not supported!");
		return ModelIndex();
	}
}

ModelIndex InnoFileSystemNS::AssetLoader::loadModelFromDisk(const char* fileName, bool AsyncUploadGPUResource)
{
	auto l_result = JSONParser::loadModelFromDisk(fileName, AsyncUploadGPUResource);
	g_pModuleManager->getAssetSystem()->recordLoadedModel(fileName, l_result);

	return l_result;
}

TextureDataComponent* InnoFileSystemNS::AssetLoader::loadTexture(const char* fileName)
{
	TextureDataComponent* l_TDC;

	if (g_pModuleManager->getAssetSystem()->findLoadedTexture(fileName, l_TDC))
	{
		return l_TDC;
	}
	else
	{
		l_TDC = TextureIO::loadTexture(fileName);
		if (l_TDC)
		{
			g_pModuleManager->getAssetSystem()->recordLoadedTexture(fileName, l_TDC);
		}
		return l_TDC;
	}
}