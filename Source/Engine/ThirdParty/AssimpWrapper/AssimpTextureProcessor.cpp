#include "AssimpTextureProcessor.h"

#include "../../Common/LogService.h"
#include "../../Common/MathHelper.h"
#include "../../Common/Randomizer.h"
#include "../../Common/IOService.h"
#include "../../Services/AssetService.h"
#include "../../Services/EntityRegistry.h"
#include "../../Services/TextureResourceService.h"
#include "../../ThirdParty/STBWrapper/STBWrapper.h"
#include "../../Engine.h"

using namespace Inno;

TextureComponent* AssimpTextureProcessor::CreateTextureComponent(const char* FileName, TextureSampler Sampler, TextureUsage Usage, bool IsSRGB, uint32_t TextureSlotIndex, const char* BaseName)
{
	Log(Verbose, "Creating TextureComponent for: ", FileName);

	if (!g_Engine->Get<IOService>()->isFileExist(FileName))
	{
		Log(Warning, "Texture file not found: ", FileName);
		return nullptr;
	}

	auto l_Name = std::string(BaseName) + "." + std::string(FileName) + "/";
	auto l_TempEntityID = g_Engine->Get<EntityRegistry>()->Spawn(ObjectLifespan::Frame, l_Name.c_str());

	auto l_TextureComponent = g_Engine->Get<TextureResourceService>()->Add(l_Name.c_str());

	l_TextureComponent->m_TextureDesc.Sampler = Sampler;
	l_TextureComponent->m_TextureDesc.Usage = Usage;
	l_TextureComponent->m_TextureDesc.IsSRGB = IsSRGB;

	void* l_TextureData = STBWrapper::Load(FileName, *l_TextureComponent);
	if (!l_TextureData)
	{
		Log(Error, "Failed to load texture data: ", FileName);
		g_Engine->Get<EntityRegistry>()->Destroy(l_TempEntityID);
		return nullptr;
	}

	bool l_Result = AssetService::Save(*l_TextureComponent, l_TextureData);

	g_Engine->Get<EntityRegistry>()->Destroy(l_TempEntityID);

	if (l_Result)
	{
		Log(Success, "Created and saved TextureComponent: ", FileName);
		return l_TextureComponent;
	}
	else
	{
		Log(Error, "Failed to save TextureComponent: ", FileName);
		return nullptr;
	}
}
