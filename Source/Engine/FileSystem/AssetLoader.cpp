#include "AssetLoader.h"

#include "stb/stb_image.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

#include "IOServices.h"
#include "JSONParser.h"

INNO_PRIVATE_SCOPE InnoFileSystemNS::AssetLoader
{
	std::unordered_map<std::string, ModelMap> m_loadedModelMap;
	std::unordered_map<std::string, TextureDataComponent*> m_loadedTexture;

	ModelMap loadModelFromDisk(const std::string & fileName);
	TextureDataComponent* loadTextureFromDisk(const std::string & fileName);
}

ModelMap InnoFileSystemNS::AssetLoader::loadModel(const std::string & fileName)
{
	auto l_extension = getFileExtension(fileName);
	if (l_extension == ".InnoModel")
	{
		ModelMap l_result;

		// check if this file has already been loaded once
		auto l_loadedModelMap = m_loadedModelMap.find(fileName);
		if (l_loadedModelMap != m_loadedModelMap.end())
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: AssetLoader: " + fileName + " has been already loaded.");
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
			return loadModelFromDisk(fileName);
		}
	}
	else
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "FileSystem: AssimpWrapper: " + fileName + " is not supported!");
		return ModelMap();
	}
}

ModelMap InnoFileSystemNS::AssetLoader::loadModelFromDisk(const std::string & fileName)
{
	auto l_result = JSONParser::loadModelFromDisk(fileName);
	m_loadedModelMap.emplace(fileName, l_result);

	return l_result;
}

TextureDataComponent* InnoFileSystemNS::AssetLoader::loadTexture(const std::string& fileName)
{
	TextureDataComponent* l_TDC;

	auto l_loadedTDC = m_loadedTexture.find(fileName);
	if (l_loadedTDC != m_loadedTexture.end())
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: AssetLoader: " + fileName + " has been already loaded.");
		l_TDC = l_loadedTDC->second;
	}
	else
	{
		l_TDC = loadTextureFromDisk(fileName);
	}

	return l_TDC;
}

TextureDataComponent* InnoFileSystemNS::AssetLoader::loadTextureFromDisk(const std::string & fileName)
{
	int width, height, nrChannels;

	// load image, flip texture
	stbi_set_flip_vertically_on_load(true);

	void* l_rawData;
	auto l_fullPath = getWorkingDirectory() + fileName;
	auto l_isHDR = stbi_is_hdr(l_fullPath.c_str());

	if (l_isHDR)
	{
		l_rawData = stbi_loadf(l_fullPath.c_str(), &width, &height, &nrChannels, 0);
	}
	else
	{
		l_rawData = stbi_load(l_fullPath.c_str(), &width, &height, &nrChannels, 0);
	}
	if (l_rawData)
	{
		auto l_TDC = g_pModuleManager->getRenderingFrontend()->addTextureDataComponent();

		l_TDC->m_componentName = fileName.c_str();

		l_TDC->m_textureDataDesc.pixelDataFormat = TexturePixelDataFormat(nrChannels - 1);
		l_TDC->m_textureDataDesc.wrapMethod = TextureWrapMethod::REPEAT;
		l_TDC->m_textureDataDesc.minFilterMethod = TextureFilterMethod::LINEAR_MIPMAP_LINEAR;
		l_TDC->m_textureDataDesc.magFilterMethod = TextureFilterMethod::LINEAR;
		l_TDC->m_textureDataDesc.pixelDataType = l_isHDR ? TexturePixelDataType::FLOAT16 : TexturePixelDataType::UBYTE;
		l_TDC->m_textureDataDesc.width = width;
		l_TDC->m_textureDataDesc.height = height;
		l_TDC->m_textureData = l_rawData;
		l_TDC->m_objectStatus = ObjectStatus::Created;

		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: AssetLoader: STB_Image: " + l_fullPath + " has been loaded.");

		m_loadedTexture.emplace(fileName, l_TDC);

		return l_TDC;
	}
	else
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: AssetLoader: STB_Image: Failed to load texture: " + l_fullPath);

		return nullptr;
	}
}