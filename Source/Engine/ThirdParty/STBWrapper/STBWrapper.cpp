#include "STBWrapper.h"

#include "../../Core/Logger.h"

#include "../../Core/IOService.h"

#include "../../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

#include "stb_image.h"
#include "stb_image_write.h"

TextureComponent* STBWrapper::loadTexture(const char* fileName)
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
		auto l_TextureComp = g_Engine->getRenderingFrontend()->addTextureComponent();
#ifdef INNO_DEBUG
        auto l_fileName = std::string(fileName);
        l_fileName += "/";
		l_TextureComp->m_InstanceName = l_fileName.c_str();
#endif
		l_TextureComp->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat(nrChannels);
		l_TextureComp->m_TextureDesc.PixelDataType = l_isHDR ? TexturePixelDataType::Float16 : TexturePixelDataType::UByte;
		l_TextureComp->m_TextureDesc.UseMipMap = true;
		l_TextureComp->m_TextureDesc.Width = width;
		l_TextureComp->m_TextureDesc.Height = height;
		l_TextureComp->m_TextureData = l_rawData;
		l_TextureComp->m_ObjectStatus = ObjectStatus::Created;

		Logger::Log(LogLevel::Verbose, "FileSystem: STBWrapper: STB_Image: ", l_fullPath.c_str(), " has been loaded.");

		return l_TextureComp;
	}
	else
	{
		Logger::Log(LogLevel::Error, "FileSystem: STBWrapper: STB_Image: Failed to load texture: ", l_fullPath.c_str());

		return nullptr;
	}
}

bool STBWrapper::saveTexture(const char* fileName, TextureComponent* TextureComp)
{
	if (TextureComp->m_TextureDesc.PixelDataType == TexturePixelDataType::Float16 || TextureComp->m_TextureDesc.PixelDataType == TexturePixelDataType::Float32)
	{
		if (TextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler1DArray)
		{
			stbi_write_hdr((IOService::getWorkingDirectory() + fileName + ".hdr").c_str(), (int32_t)TextureComp->m_TextureDesc.Width, 1, (int32_t)TextureComp->m_TextureDesc.PixelDataFormat, (float*)TextureComp->m_TextureData);
		}
		else if (TextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler2DArray
			|| TextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
		{
			stbi_write_hdr((IOService::getWorkingDirectory() + fileName + ".hdr").c_str(), (int32_t)TextureComp->m_TextureDesc.Width, (int32_t)TextureComp->m_TextureDesc.Height * TextureComp->m_TextureDesc.DepthOrArraySize, (int32_t)TextureComp->m_TextureDesc.PixelDataFormat, (float*)TextureComp->m_TextureData);
		}
		else if (TextureComp->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap)
		{
			stbi_write_hdr((IOService::getWorkingDirectory() + fileName + ".hdr").c_str(), (int32_t)TextureComp->m_TextureDesc.Width, (int32_t)TextureComp->m_TextureDesc.Height * 6, (int32_t)TextureComp->m_TextureDesc.PixelDataFormat, (float*)TextureComp->m_TextureData);
		}
		else
		{
			stbi_write_hdr((IOService::getWorkingDirectory() + fileName + ".hdr").c_str(), (int32_t)TextureComp->m_TextureDesc.Width, (int32_t)TextureComp->m_TextureDesc.Height, (int32_t)TextureComp->m_TextureDesc.PixelDataFormat, (float*)TextureComp->m_TextureData);
		}
	}
	else
	{
		if (TextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler1DArray)
		{
			stbi_write_png((IOService::getWorkingDirectory() + fileName + ".png").c_str(), (int32_t)TextureComp->m_TextureDesc.Width, 1, (int32_t)TextureComp->m_TextureDesc.PixelDataFormat, TextureComp->m_TextureData, (int32_t)TextureComp->m_TextureDesc.Width * sizeof(int32_t));
		}
		else if (TextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler2DArray
			|| TextureComp->m_TextureDesc.Sampler == TextureSampler::Sampler3D)
		{
			stbi_write_png((IOService::getWorkingDirectory() + fileName + ".png").c_str(), (int32_t)TextureComp->m_TextureDesc.Width, (int32_t)TextureComp->m_TextureDesc.Height * TextureComp->m_TextureDesc.DepthOrArraySize, (int32_t)TextureComp->m_TextureDesc.PixelDataFormat, TextureComp->m_TextureData, (int32_t)TextureComp->m_TextureDesc.Width * sizeof(int32_t));
		}
		else if (TextureComp->m_TextureDesc.Sampler == TextureSampler::SamplerCubemap)
		{
			stbi_write_png((IOService::getWorkingDirectory() + fileName + ".png").c_str(), (int32_t)TextureComp->m_TextureDesc.Width, (int32_t)TextureComp->m_TextureDesc.Height * 6, (int32_t)TextureComp->m_TextureDesc.PixelDataFormat, TextureComp->m_TextureData, (int32_t)TextureComp->m_TextureDesc.Width * sizeof(int32_t));
		}
		else
		{
			stbi_write_png((IOService::getWorkingDirectory() + fileName + ".png").c_str(), (int32_t)TextureComp->m_TextureDesc.Width, (int32_t)TextureComp->m_TextureDesc.Height, (int32_t)TextureComp->m_TextureDesc.PixelDataFormat, TextureComp->m_TextureData, (int32_t)TextureComp->m_TextureDesc.Width * sizeof(int32_t));
		}
	}

	return true;
}
