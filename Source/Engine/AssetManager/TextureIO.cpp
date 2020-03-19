#include "TextureIO.h"

#include "../Core/InnoLogger.h"

#include "../Core/IOService.h"

#include "../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

TextureDataComponent* InnoFileSystemNS::TextureIO::loadTexture(const char* fileName)
{
	int32_t width, height, nrChannels;

	// load image, flip texture
	stbi_set_flip_vertically_on_load(true);

	void* l_rawData;
	auto l_fullPath = IOService::getWorkingDirectory() + fileName;
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

		l_TDC->m_ComponentName = (std::string(fileName) + "/").c_str();

		l_TDC->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat(nrChannels - 1);
		l_TDC->m_TextureDesc.PixelDataType = l_isHDR ? TexturePixelDataType::Float16 : TexturePixelDataType::UByte;
		l_TDC->m_TextureDesc.UseMipMap = true;
		l_TDC->m_TextureDesc.Width = width;
		l_TDC->m_TextureDesc.Height = height;
		l_TDC->m_TextureData = l_rawData;
		l_TDC->m_ObjectStatus = ObjectStatus::Created;

		InnoLogger::Log(LogLevel::Verbose, "FileSystem: TextureIO: STB_Image: ", l_fullPath.c_str(), " has been loaded.");

		return l_TDC;
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "FileSystem: TextureIO: STB_Image: Failed to load texture: ", l_fullPath.c_str());

		return nullptr;
	}
}

bool InnoFileSystemNS::TextureIO::saveTexture(const char* fileName, TextureDataComponent * TDC)
{
	if (TDC->m_TextureDesc.PixelDataType == TexturePixelDataType::Float16 || TDC->m_TextureDesc.PixelDataType == TexturePixelDataType::Float32)
	{
		if (TDC->m_TextureDesc.Sampler == TextureSampler::Sampler1DArray)
		{
			stbi_write_hdr((IOService::getWorkingDirectory() + fileName + ".hdr").c_str(), (int32_t)TDC->m_TextureDesc.Width, 1, (int32_t)TDC->m_TextureDesc.PixelDataFormat + 1, (float*)TDC->m_TextureData);
		}
		else if (TDC->m_TextureDesc.Sampler == TextureSampler::Sampler2DArray
			|| TDC->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
		{
			stbi_write_hdr((IOService::getWorkingDirectory() + fileName + ".hdr").c_str(), (int32_t)TDC->m_TextureDesc.Width, (int32_t)TDC->m_TextureDesc.Height * TDC->m_TextureDesc.DepthOrArraySize, (int32_t)TDC->m_TextureDesc.PixelDataFormat + 1, (float*)TDC->m_TextureData);
		}
		else if (TDC->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap)
		{
			stbi_write_hdr((IOService::getWorkingDirectory() + fileName + ".hdr").c_str(), (int32_t)TDC->m_TextureDesc.Width, (int32_t)TDC->m_TextureDesc.Height * 6, (int32_t)TDC->m_TextureDesc.PixelDataFormat + 1, (float*)TDC->m_TextureData);
		}
		else
		{
			stbi_write_hdr((IOService::getWorkingDirectory() + fileName + ".hdr").c_str(), (int32_t)TDC->m_TextureDesc.Width, (int32_t)TDC->m_TextureDesc.Height, (int32_t)TDC->m_TextureDesc.PixelDataFormat + 1, (float*)TDC->m_TextureData);
		}
	}
	else
	{
		if (TDC->m_TextureDesc.Sampler == TextureSampler::Sampler1DArray)
		{
			stbi_write_png((IOService::getWorkingDirectory() + fileName + ".png").c_str(), (int32_t)TDC->m_TextureDesc.Width, 1, (int32_t)TDC->m_TextureDesc.PixelDataFormat + 1, TDC->m_TextureData, (int32_t)TDC->m_TextureDesc.Width * sizeof(int32_t));
		}
		else if (TDC->m_TextureDesc.Sampler == TextureSampler::Sampler2DArray
			|| TDC->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
		{
			stbi_write_png((IOService::getWorkingDirectory() + fileName + ".png").c_str(), (int32_t)TDC->m_TextureDesc.Width, (int32_t)TDC->m_TextureDesc.Height * TDC->m_TextureDesc.DepthOrArraySize, (int32_t)TDC->m_TextureDesc.PixelDataFormat + 1, TDC->m_TextureData, (int32_t)TDC->m_TextureDesc.Width * sizeof(int32_t));
		}
		else if (TDC->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap)
		{
			stbi_write_png((IOService::getWorkingDirectory() + fileName + ".png").c_str(), (int32_t)TDC->m_TextureDesc.Width, (int32_t)TDC->m_TextureDesc.Height * 6, (int32_t)TDC->m_TextureDesc.PixelDataFormat + 1, TDC->m_TextureData, (int32_t)TDC->m_TextureDesc.Width * sizeof(int32_t));
		}
		else
		{
			stbi_write_png((IOService::getWorkingDirectory() + fileName + ".png").c_str(), (int32_t)TDC->m_TextureDesc.Width, (int32_t)TDC->m_TextureDesc.Height, (int32_t)TDC->m_TextureDesc.PixelDataFormat + 1, TDC->m_TextureData, (int32_t)TDC->m_TextureDesc.Width * sizeof(int32_t));
		}
	}

	return true;
}