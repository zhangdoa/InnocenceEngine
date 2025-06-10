#include "AssimpTextureProcessor.h"

#include "../../Common/LogService.h"
#include "../../Common/MathHelper.h"
#include "../../Common/Randomizer.h"
#include "../../Common/IOService.h"
#include "../../Services/AssetService.h"
#include "../../Services/ComponentManager.h"
#include "../../Services/EntityManager.h"
#include "../../ThirdParty/STBWrapper/STBWrapper.h"
#include "../../Engine.h"

using namespace Inno;

TextureComponent* AssimpTextureProcessor::CreateTextureComponent(const char* fileName, TextureSampler sampler, TextureUsage usage, bool IsSRGB, uint32_t textureSlotIndex, const char* baseName)
{
	Log(Verbose, "Creating TextureComponent for: ", fileName);
	
	if (!g_Engine->Get<IOService>()->isFileExist(fileName))
	{
		Log(Warning, "Texture file not found: ", fileName);
		return nullptr;
	}
	
	auto l_name = std::string(baseName) + "." + std::string(fileName) + "/";
	auto l_tempEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Frame, l_name.c_str());

	auto l_textureComponent = g_Engine->Get<ComponentManager>()->Spawn<TextureComponent>(l_tempEntity, true, ObjectLifespan::Scene);

	l_textureComponent->m_TextureDesc.Sampler = sampler;
	l_textureComponent->m_TextureDesc.Usage = usage;
	l_textureComponent->m_TextureDesc.IsSRGB = IsSRGB;

	void* l_textureData = STBWrapper::Load(fileName, *l_textureComponent);	
	if (!l_textureData)
	{
		Log(Error, "Failed to load texture data: ", fileName);
		g_Engine->Get<EntityManager>()->Destroy(l_tempEntity);
		return nullptr;
	}

	bool result = AssetService::Save(*l_textureComponent, l_textureData);

	g_Engine->Get<EntityManager>()->Destroy(l_tempEntity);
	
	if (result)
	{
		Log(Success, "Created and saved TextureComponent: ", fileName);
		return l_textureComponent;
	}
	else
	{
		Log(Error, "Failed to save TextureComponent: ", fileName);
		return nullptr;
	}
}
