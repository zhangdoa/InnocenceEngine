#include "STBWrapper.h"

#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Services/RenderingContextService.h"

#include "../../Engine.h"
using namespace Inno;
;

#include "stb_image.h"
#include "stb_image_write.h"

TextureComponent* STBWrapper::LoadTexture(const char* fileName)
{
	int32_t width, height, nrChannels;

	// load image, flip texture
	stbi_set_flip_vertically_on_load(true);

	void* l_rawData;
	auto l_fullPath = g_Engine->Get<IOService>()->getWorkingDirectory() + fileName;
	auto l_isHDR = stbi_is_hdr(l_fullPath.c_str());

	if (l_isHDR)
	{
		l_rawData = stbi_loadf(l_fullPath.c_str(), &width, &height, &nrChannels, 4);
	}
	else
	{
		l_rawData = stbi_load(l_fullPath.c_str(), &width, &height, &nrChannels, 4);
	}
	if (l_rawData)
	{
		auto l_TextureComp = g_Engine->getRenderingServer()->AddTextureComponent();
#ifdef INNO_DEBUG
        auto l_fileName = std::string(fileName);
        l_fileName += "/";
		l_TextureComp->m_InstanceName = l_fileName.c_str();
#endif
		// @TODO: Forcing RGBA is wasting memory
		l_TextureComp->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat(4);
		l_TextureComp->m_TextureDesc.PixelDataType = l_isHDR ? TexturePixelDataType::Float16 : TexturePixelDataType::UByte;
		l_TextureComp->m_TextureDesc.UseMipMap = true;
		l_TextureComp->m_TextureDesc.Width = width;
		l_TextureComp->m_TextureDesc.Height = height;
		l_TextureComp->m_InitialData = l_rawData;
		l_TextureComp->m_ObjectStatus = ObjectStatus::Created;

		Log(Verbose, "STBWrapper: STB_Image: ", l_fullPath.c_str(), " has been loaded.");

		return l_TextureComp;
	}
	else
	{
		Log(Error, "STBWrapper: STB_Image: Failed to load texture: ", l_fullPath.c_str());

		return nullptr;
	}
}

bool STBWrapper::SaveTexture(const char* fileName, const TextureDesc& textureDesc, void* textureData)
{
	int result = 1;
	int32_t comp = (int32_t)textureDesc.PixelDataFormat;

	// BGRA, D and DS, all 4 * sizeof(float)
	comp = comp > 4 ? 4 : comp;

	if (textureDesc.PixelDataType == TexturePixelDataType::Float16 || textureDesc.PixelDataType == TexturePixelDataType::Float32)
	{
		if (textureDesc.Sampler == TextureSampler::Sampler1DArray)
		{
			result = stbi_write_hdr((g_Engine->Get<IOService>()->getWorkingDirectory() + fileName + ".hdr").c_str(), (int32_t)textureDesc.Width, 1, comp, (float*)textureData);
		}
		else if (textureDesc.Sampler == TextureSampler::Sampler2DArray
			|| textureDesc.Sampler == TextureSampler::Sampler3D)
		{
			result = stbi_write_hdr((g_Engine->Get<IOService>()->getWorkingDirectory() + fileName + ".hdr").c_str(), (int32_t)textureDesc.Width, (int32_t)textureDesc.Height * textureDesc.DepthOrArraySize, comp, (float*)textureData);
		}
		else if (textureDesc.Sampler == TextureSampler::SamplerCubemap)
		{
			result = stbi_write_hdr((g_Engine->Get<IOService>()->getWorkingDirectory() + fileName + ".hdr").c_str(), (int32_t)textureDesc.Width, (int32_t)textureDesc.Height * 6, comp, (float*)textureData);
		}
		else
		{
			result = stbi_write_hdr((g_Engine->Get<IOService>()->getWorkingDirectory() + fileName + ".hdr").c_str(), (int32_t)textureDesc.Width, (int32_t)textureDesc.Height, comp, (float*)textureData);
		}
	}
	else
	{
		if (textureDesc.Sampler == TextureSampler::Sampler1DArray)
		{
			result = stbi_write_png((g_Engine->Get<IOService>()->getWorkingDirectory() + fileName + ".png").c_str(), (int32_t)textureDesc.Width, 1, comp, textureData, (int32_t)textureDesc.Width * sizeof(int32_t));
		}
		else if (textureDesc.Sampler == TextureSampler::Sampler2DArray
			|| textureDesc.Sampler == TextureSampler::Sampler3D)
		{
			result = stbi_write_png((g_Engine->Get<IOService>()->getWorkingDirectory() + fileName + ".png").c_str(), (int32_t)textureDesc.Width, (int32_t)textureDesc.Height * textureDesc.DepthOrArraySize, comp, textureData, (int32_t)textureDesc.Width * sizeof(int32_t));
		}
		else if (textureDesc.Sampler == TextureSampler::SamplerCubemap)
		{
			result = stbi_write_png((g_Engine->Get<IOService>()->getWorkingDirectory() + fileName + ".png").c_str(), (int32_t)textureDesc.Width, (int32_t)textureDesc.Height * 6, comp, textureData, (int32_t)textureDesc.Width * sizeof(int32_t));
		}
		else
		{
			result = stbi_write_png((g_Engine->Get<IOService>()->getWorkingDirectory() + fileName + ".png").c_str(), (int32_t)textureDesc.Width, (int32_t)textureDesc.Height, comp, textureData, (int32_t)textureDesc.Width * sizeof(int32_t));
		}
	}

	return result == 1;
}