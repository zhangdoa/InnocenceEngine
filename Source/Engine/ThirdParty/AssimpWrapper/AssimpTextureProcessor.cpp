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

TextureComponent* AssimpTextureProcessor::CreateTextureComponent(const char* fileName, TextureSampler sampler, TextureUsage usage, bool IsSRGB, uint32_t textureSlotIndex, const char* exportName)
{
	Log(Verbose, "Creating TextureComponent for: ", fileName);
	
	// Check if texture file exists
	auto l_fullPath = g_Engine->Get<IOService>()->getWorkingDirectory() + fileName;
	if (!g_Engine->Get<IOService>()->isFileExist(l_fullPath.c_str()))
	{
		Log(Warning, "Texture file not found: ", l_fullPath.c_str());
		return nullptr;
	}
	
	// Create temporary entity using EntityManager
	auto l_tempEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Frame, "TempTextureEntity");
	
	// Create TextureComponent via ComponentManager
	auto l_textureComponent = g_Engine->Get<ComponentManager>()->Spawn<TextureComponent>(l_tempEntity, true, ObjectLifespan::Scene);
	l_textureComponent->m_UUID = Randomizer::GenerateUUID();
	l_textureComponent->m_ObjectStatus = ObjectStatus::Created;
	
	// Set texture properties
	l_textureComponent->m_TextureDesc.Sampler = sampler;
	l_textureComponent->m_TextureDesc.Usage = usage;
	l_textureComponent->m_TextureDesc.IsSRGB = IsSRGB;
	
	// Load texture data using STBWrapper
	void* l_textureData = STBWrapper::Load(fileName, *l_textureComponent);
	
	if (!l_textureData)
	{
		Log(Error, "Failed to load texture data: ", fileName);
		g_Engine->Get<EntityManager>()->Destroy(l_tempEntity);
		return nullptr;
	}
	
	// Save TextureComponent without subfolders and using GetTypeName()
	auto l_baseName = g_Engine->Get<IOService>()->getFileName(fileName);
	auto l_workingDir = g_Engine->Get<IOService>()->getWorkingDirectory();
	auto l_textureName = std::string(exportName) + "_texture_" + l_baseName;
	auto l_texturePath = l_workingDir + "Data/Components/" + l_textureName + "." + TextureComponent::GetTypeName() + ".inno";
	
	bool result = AssetService::Save(l_texturePath.c_str(), *l_textureComponent, l_textureData);
	
	// Clean up temporary entity
	g_Engine->Get<EntityManager>()->Destroy(l_tempEntity);
	
	if (result)
	{
		Log(Success, "Created and saved TextureComponent: ", l_texturePath.c_str());
		return l_textureComponent;
	}
	else
	{
		Log(Error, "Failed to save TextureComponent: ", l_texturePath.c_str());
		return nullptr;
	}
}
