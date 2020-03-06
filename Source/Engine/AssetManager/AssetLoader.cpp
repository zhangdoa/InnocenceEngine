#include "AssetLoader.h"

#include "../../Engine/Core/InnoLogger.h"

#include "../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

#include "../Core/IOService.h"
#include "JSONParser.h"
#include "TextureIO.h"

namespace InnoFileSystemNS::AssetLoader
{
	Model* loadModelFromDisk(const char* fileName, bool AsyncUploadGPUResource = true);
}

Model* InnoFileSystemNS::AssetLoader::loadModel(const char* fileName, bool AsyncUploadGPUResource)
{
	auto l_extension = IOService::getFileExtension(fileName);
	if (l_extension == ".InnoModel")
	{
		Model* l_loadedModel;

		if (g_pModuleManager->getAssetSystem()->findLoadedModel(fileName, l_loadedModel))
		{
			return l_loadedModel;
		}
		else
		{
			return loadModelFromDisk(fileName, AsyncUploadGPUResource);
		}
	}
	else
	{
		InnoLogger::Log(LogLevel::Warning, "FileSystem: AssimpWrapper: ", fileName, " is not supported!");
		return nullptr;
	}
}

Model* InnoFileSystemNS::AssetLoader::loadModelFromDisk(const char* fileName, bool AsyncUploadGPUResource)
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