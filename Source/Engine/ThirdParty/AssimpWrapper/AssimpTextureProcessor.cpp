#include "AssimpTextureProcessor.h"

#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Services/AssetService.h"
#include "../../ThirdParty/STBWrapper/STBWrapper.h"
#include "../../Engine.h"

using namespace Inno;

std::string AssimpTextureProcessor::CreateTextureComponent(const char* FileName, TextureSampler Sampler, TextureUsage Usage, bool IsSRGB, uint32_t TextureSlotIndex, const char* BaseName)
{
	Log(Verbose, "Creating TextureComponent for: ", FileName);

	if (!g_Engine->Get<IOService>()->isFileExist(FileName))
	{
		Log(Warning, "Texture file not found: ", FileName);
		return {};
	}

	auto l_Name = std::string(BaseName) + "." + std::string(FileName);

	TextureComponent l_Texture = {};
	l_Texture.m_InstanceName = l_Name.c_str();
	l_Texture.m_TextureDesc.Sampler = Sampler;
	l_Texture.m_TextureDesc.Usage = Usage;
	l_Texture.m_TextureDesc.IsSRGB = IsSRGB;

	void* l_TextureData = STBWrapper::Load(FileName, l_Texture);
	if (!l_TextureData)
	{
		Log(Error, "Failed to load texture data: ", FileName);
		return {};
	}

	bool l_Result = AssetService::Save(l_Texture, l_TextureData);

	if (l_Result)
	{
		Log(Success, "Created and saved TextureComponent: ", FileName);
		return l_Name;
	}

	Log(Error, "Failed to save TextureComponent: ", FileName);
	return {};
}
