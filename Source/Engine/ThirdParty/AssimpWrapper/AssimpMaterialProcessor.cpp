#include "AssimpMaterialProcessor.h"
#include "AssimpTextureProcessor.h"

#include "assimp/material.h"
#include "../../Common/LogService.h"
#include "../../Common/MathHelper.h"
#include "../../Common/Randomizer.h"
#include "../../Services/AssetService.h"
#include "../../Engine.h"

using namespace Inno;

bool AssimpMaterialProcessor::CreateMaterialComponent(const aiMaterial* Material, const char* BaseName, MaterialComponent& OutMaterial)
{
	auto l_MaterialName = Material->GetName().C_Str();
	Log(Verbose, "Creating MaterialComponent for: ", l_MaterialName);

	OutMaterial = {};
	auto l_InstanceName = std::string(BaseName) + "." + l_MaterialName + ".MaterialComponent/";
	OutMaterial.m_InstanceName = l_InstanceName.c_str();

	auto l_handle = AssetService::AllocateMaterialAsset(l_MaterialName, ObjectLifespan::Scene);
	OutMaterial.m_Asset = l_handle;

	auto* l_assetData = AssetService::GetMaterialAsset(l_handle);
	if (!l_assetData)
	{
		Log(Error, "Failed to allocate MaterialAsset for: ", l_MaterialName);
		return false;
	}

	ProcessMaterialProperties(Material, l_assetData);

	ProcessMaterialTextures(Material, BaseName, l_assetData);

	l_assetData->m_Residency = AssetResidency::Resident;

	bool l_Result = AssetService::Save(OutMaterial);

	if (l_Result)
	{
		Log(Success, "Created and saved MaterialComponent: ", l_MaterialName);
	}
	else
	{
		Log(Error, "Failed to save MaterialComponent: ", l_MaterialName);
	}

	return l_Result;
}

void AssimpMaterialProcessor::ProcessMaterialProperties(const aiMaterial* material, MaterialAssetData* assetData)
{
	aiColor3D l_result;

	if (material->Get(AI_MATKEY_COLOR_DIFFUSE, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		assetData->m_Attributes.AlbedoR = l_result.r;
		assetData->m_Attributes.AlbedoG = l_result.g;
		assetData->m_Attributes.AlbedoB = l_result.b;
	}
	else
	{
		assetData->m_Attributes.AlbedoR = 1.0f;
		assetData->m_Attributes.AlbedoG = 1.0f;
		assetData->m_Attributes.AlbedoB = 1.0f;
	}

	if (material->Get(AI_MATKEY_COLOR_TRANSPARENT, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		assetData->m_Attributes.Alpha = l_result.r;
		assetData->m_ShaderModel = ShaderModel::Transparent;
	}
	else
	{
		assetData->m_Attributes.Alpha = 1.0f;
		assetData->m_ShaderModel = ShaderModel::Opaque;
	}

	if (material->Get(AI_MATKEY_COLOR_SPECULAR, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		assetData->m_Attributes.Metallic = l_result.r;
	}
	else
	{
		assetData->m_Attributes.Metallic = 0.5f;
	}

	if (material->Get(AI_MATKEY_SHININESS, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		assetData->m_Attributes.Roughness = l_result.r;
	}
	else
	{
		assetData->m_Attributes.Roughness = 0.5f;
	}

	if (material->Get(AI_MATKEY_COLOR_AMBIENT, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		assetData->m_Attributes.AO = l_result.r;
	}
	else
	{
		assetData->m_Attributes.AO = 0.0f;
	}

	if (material->Get(AI_MATKEY_COLOR_REFLECTIVE, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		assetData->m_Attributes.Thickness = l_result.r;
	}
	else
	{
		assetData->m_Attributes.Thickness = 1.0f;
	}
}

void AssimpMaterialProcessor::ProcessMaterialTextures(const aiMaterial* material, const char* baseName, MaterialAssetData* assetData)
{
	assetData->m_TextureNames.clear();

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
				assetData->m_TextureNames.emplace_back(l_textureComponent->m_InstanceName.c_str());
		}
	}
}
