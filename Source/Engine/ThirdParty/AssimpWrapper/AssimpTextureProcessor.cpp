// TODO Phase2-migrate: TextureComponent still inherits from Component/GPUResourceComponent,
// so it cannot be emplaced in EntityRegistry yet. Keep using ComponentManager/EntityManager
// until TextureComponent is stripped to a plain struct.
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

TextureComponent* AssimpTextureProcessor::CreateTextureComponent(const char* FileName, TextureSampler Sampler, TextureUsage Usage, bool IsSRGB, uint32_t TextureSlotIndex, const char* BaseName)
{
	Log(Verbose, "Creating TextureComponent for: ", FileName);

	if (!g_Engine->Get<IOService>()->isFileExist(FileName))
	{
		Log(Warning, "Texture file not found: ", FileName);
		return nullptr;
	}

	auto l_Name = std::string(BaseName) + "." + std::string(FileName) + "/";
	auto l_TempEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Frame, l_Name.c_str());

	auto l_TextureComponent = g_Engine->Get<ComponentManager>()->Spawn<TextureComponent>(l_TempEntity, true, ObjectLifespan::Scene);

	l_TextureComponent->m_TextureDesc.Sampler = Sampler;
	l_TextureComponent->m_TextureDesc.Usage = Usage;
	l_TextureComponent->m_TextureDesc.IsSRGB = IsSRGB;

	void* l_TextureData = STBWrapper::Load(FileName, *l_TextureComponent);
	if (!l_TextureData)
	{
		Log(Error, "Failed to load texture data: ", FileName);
		g_Engine->Get<EntityManager>()->Destroy(l_TempEntity);
		return nullptr;
	}

	bool l_Result = AssetService::Save(*l_TextureComponent, l_TextureData);

	g_Engine->Get<EntityManager>()->Destroy(l_TempEntity);

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
