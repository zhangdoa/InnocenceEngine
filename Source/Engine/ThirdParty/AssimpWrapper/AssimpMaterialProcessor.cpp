#include "AssimpMaterialProcessor.h"
#include "AssimpTextureProcessor.h"

#include "assimp/material.h"
#include "../../Common/LogService.h"
#include "../../Common/MathHelper.h"
#include "../../Common/Randomizer.h"
#include "../../Services/AssetService.h"
#include "../../Services/ComponentManager.h"
#include "../../Services/EntityManager.h"
#include "../../Engine.h"

using namespace Inno;

MaterialComponent* AssimpMaterialProcessor::CreateMaterialComponent(const aiMaterial* material, const char* baseName)
{
	auto l_materialName = material->GetName().C_Str();
	Log(Verbose, "Creating MaterialComponent for: ", l_materialName);

	auto l_name = std::string(baseName) + "." + std::string(l_materialName) + "/";
	auto l_tempEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Frame, l_name.c_str());

	auto l_materialComponent = g_Engine->Get<ComponentManager>()->Spawn<MaterialComponent>(l_tempEntity, true, ObjectLifespan::Frame);

	ProcessMaterialProperties(material, l_materialComponent);

	ProcessMaterialTextures(material, baseName, l_materialComponent);

	bool result = AssetService::Save(*l_materialComponent);

	g_Engine->Get<EntityManager>()->Destroy(l_tempEntity);

	if (result)
	{
		Log(Success, "Created and saved MaterialComponent: ", l_materialName);
		return l_materialComponent;
	}
	else
	{
		Log(Error, "Failed to save MaterialComponent: ", l_materialName);
		return nullptr;
	}
}

void AssimpMaterialProcessor::ProcessMaterialProperties(const aiMaterial* material, MaterialComponent* materialComponent)
{
	aiColor3D l_result;

	// Albedo (Diffuse color)
	if (material->Get(AI_MATKEY_COLOR_DIFFUSE, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		materialComponent->m_materialAttributes.AlbedoR = l_result.r;
		materialComponent->m_materialAttributes.AlbedoG = l_result.g;
		materialComponent->m_materialAttributes.AlbedoB = l_result.b;
	}
	else
	{
		materialComponent->m_materialAttributes.AlbedoR = 1.0f;
		materialComponent->m_materialAttributes.AlbedoG = 1.0f;
		materialComponent->m_materialAttributes.AlbedoB = 1.0f;
	}

	// Alpha (Transparency)
	if (material->Get(AI_MATKEY_COLOR_TRANSPARENT, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		materialComponent->m_materialAttributes.Alpha = l_result.r;
		materialComponent->m_ShaderModel = ShaderModel::Transparent;
	}
	else
	{
		materialComponent->m_materialAttributes.Alpha = 1.0f;
		materialComponent->m_ShaderModel = ShaderModel::Opaque;
	}

	// Metallic (Specular)
	if (material->Get(AI_MATKEY_COLOR_SPECULAR, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		materialComponent->m_materialAttributes.Metallic = l_result.r;
	}
	else
	{
		materialComponent->m_materialAttributes.Metallic = 0.5f;
	}

	// Roughness (Shininess)
	if (material->Get(AI_MATKEY_SHININESS, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		materialComponent->m_materialAttributes.Roughness = l_result.r;
	}
	else
	{
		materialComponent->m_materialAttributes.Roughness = 0.5f;
	}

	// AO (Ambient)
	if (material->Get(AI_MATKEY_COLOR_AMBIENT, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		materialComponent->m_materialAttributes.AO = l_result.r;
	}
	else
	{
		materialComponent->m_materialAttributes.AO = 0.0f;
	}

	// Thickness (Reflective)
	if (material->Get(AI_MATKEY_COLOR_REFLECTIVE, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		materialComponent->m_materialAttributes.Thickness = l_result.r;
	}
	else
	{
		materialComponent->m_materialAttributes.Thickness = 1.0f;
	}
}

void AssimpMaterialProcessor::ProcessMaterialTextures(const aiMaterial* material, const char* baseName, MaterialComponent* materialComponent)
{
	materialComponent->m_TextureComponents.clear();

	for (uint32_t i = 0; i < aiTextureType_UNKNOWN; i++)
	{
		auto l_aiTextureType = aiTextureType(i);
		if (material->GetTextureCount(l_aiTextureType) > 0)
		{
			aiString l_AssString;
			material->GetTexture(l_aiTextureType, 0, &l_AssString);
			auto l_localPath = l_AssString.C_Str();

			if (l_aiTextureType == aiTextureType::aiTextureType_NONE)
			{
				Log(Warning, l_AssString.C_Str(), " is unknown texture type!");
				continue;
			}

			TextureSampler l_sampler = TextureSampler::Sampler2D;
			TextureUsage l_usage = TextureUsage::Sample;
			bool l_isSRGB = false;
			uint32_t l_textureSlotIndex = 0;

			if (l_aiTextureType == aiTextureType::aiTextureType_HEIGHT ||
				l_aiTextureType == aiTextureType::aiTextureType_NORMALS ||
				l_aiTextureType == aiTextureType::aiTextureType_NORMAL_CAMERA)
			{
				l_usage = TextureUsage::Sample;
				l_isSRGB = false;
				l_textureSlotIndex = 0;
			}
			else if (l_aiTextureType == aiTextureType::aiTextureType_DIFFUSE ||
				l_aiTextureType == aiTextureType::aiTextureType_BASE_COLOR)
			{
				l_usage = TextureUsage::Sample;
				l_isSRGB = true;
				l_textureSlotIndex = 1;
			}
			else if (l_aiTextureType == aiTextureType::aiTextureType_SPECULAR ||
				l_aiTextureType == aiTextureType::aiTextureType_METALNESS)
			{
				l_usage = TextureUsage::Sample;
				l_isSRGB = false;
				l_textureSlotIndex = 2;
			}
			else if (l_aiTextureType == aiTextureType::aiTextureType_SHININESS ||
				l_aiTextureType == aiTextureType::aiTextureType_DIFFUSE_ROUGHNESS)
			{
				l_usage = TextureUsage::Sample;
				l_isSRGB = false;
				l_textureSlotIndex = 3;
			}
			else if (l_aiTextureType == aiTextureType::aiTextureType_AMBIENT)
			{
				l_usage = TextureUsage::Sample;
				l_isSRGB = false;
				l_textureSlotIndex = 4;
			}
			else
			{
				Log(Warning, l_AssString.C_Str(), " is unsupported texture type!");
				continue;
			}

			auto l_textureComponent = AssimpTextureProcessor::CreateTextureComponent(l_localPath, l_sampler, l_usage, l_isSRGB, l_textureSlotIndex, baseName);
			if (l_textureComponent)
				materialComponent->m_TextureComponents.emplace_back(l_textureComponent->m_UUID);
		}
	}
}
