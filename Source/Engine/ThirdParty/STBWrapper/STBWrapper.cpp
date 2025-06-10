#include "STBWrapper.h"

#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Services/RenderingContextService.h"

#include "../../Engine.h"
using namespace Inno;

#include "stb_image.h"
#include "stb_image_write.h"

void* STBWrapper::Load(const char* fileName, TextureComponent& component)
{
	int32_t width, height, nrChannels;

	// load image, flip texture
	stbi_set_flip_vertically_on_load(true);

	void* l_rawData;
	auto l_isHDR = stbi_is_hdr(fileName);

	if (l_isHDR)
	{
		l_rawData = stbi_loadf(fileName, &width, &height, &nrChannels, 4);
	}
	else
	{
		l_rawData = stbi_load(fileName, &width, &height, &nrChannels, 4);
	}

	if (!l_rawData)
	{
		Log(Error, "Failed to load texture: ", fileName);
		return nullptr;
	}

	// @TODO: Forcing RGBA is wasting memory
	component.m_TextureDesc.PixelDataFormat = TexturePixelDataFormat(4);
	component.m_TextureDesc.PixelDataType = l_isHDR ? TexturePixelDataType::Float16 : TexturePixelDataType::UByte;
	component.m_TextureDesc.MipLevels = 4;
	component.m_TextureDesc.Width = width;
	component.m_TextureDesc.Height = height;
	component.m_ObjectStatus = ObjectStatus::Created;

	Log(Verbose, "STBWrapper: STB_Image: ", fileName, " has been loaded.");

	return l_rawData;
}

bool STBWrapper::Save(const char* fileName, const TextureDesc& textureDesc, void* textureData)
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