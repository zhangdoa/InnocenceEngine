#include "AssimpMaterialProcessor.h"
#include "AssimpTextureProcessor.h"

#include "assimp/material.h"
#include "../../Common/LogService.h"
#include "../../Common/MathHelper.h"
#include "../../Common/Randomizer.h"
#include "../../Services/AssetService.h"
#include "../../Engine.h"

using namespace Inno;

namespace
{
	struct TextureSlotMapping
	{
		aiTextureType        m_AiType;
		uint32_t             m_SlotIndex;
		bool                 m_IsSRGB;
		TextureChannelSource m_BC4Source;
	};

	// Slot layout matches opaqueGeometryProcessPass.frag / m_TextureIndices_N in common.hlsl:
	// 0=normal 1=albedo 2=metallic 3=roughness 4=AO. BC4 channel-source picks the glTF
	// MetallicRoughness packing (B=metallic, G=roughness) for the two slots Assimp reports
	// off one packed RGBA; R for separate single-channel textures STB broadcasts to RGBA.
	constexpr TextureSlotMapping kTextureSlotMappings[] = {
		{ aiTextureType_HEIGHT,            0, false, TextureChannelSource::R },
		{ aiTextureType_NORMALS,           0, false, TextureChannelSource::R },
		{ aiTextureType_NORMAL_CAMERA,     0, false, TextureChannelSource::R },
		{ aiTextureType_DIFFUSE,           1, true,  TextureChannelSource::R },
		{ aiTextureType_BASE_COLOR,        1, true,  TextureChannelSource::R },
		{ aiTextureType_METALNESS,         2, false, TextureChannelSource::B },
		{ aiTextureType_SPECULAR,          2, false, TextureChannelSource::R },
		{ aiTextureType_DIFFUSE_ROUGHNESS, 3, false, TextureChannelSource::G },
		{ aiTextureType_SHININESS,         3, false, TextureChannelSource::R },
		{ aiTextureType_AMBIENT,           4, false, TextureChannelSource::R },
	};
}

bool AssimpMaterialProcessor::CreateMaterialComponent(const aiMaterial* Material, uint32_t MaterialIndex, const char* BaseName, const char* ModelBaseDir, MaterialComponent& OutMaterial)
{
	// aiMaterial::GetName() returns aiString BY VALUE. Calling .C_Str() on that
	// temporary hands out a pointer into the temporary's inline data[] buffer,
	// which dies at the semicolon — subsequent use is UB (and the prior code,
	// `auto l_MaterialName = Material->GetName().C_Str();`, hit exactly this).
	// Hold the aiString alive locally and copy into a std::string that owns
	// its storage. Same for empty-name fallback: some glTF files (Intel Sponza
	// curtains pack) ship materials with empty names; synthesize a unique
	// `material_{index}` so N unnamed materials don't collapse into one asset.
	aiString l_AiName = Material->GetName();
	std::string l_MaterialName;
	const bool l_WasEmpty = (l_AiName.length == 0);
	if (l_WasEmpty)
		l_MaterialName = "material_" + std::to_string(MaterialIndex);
	else
		l_MaterialName.assign(l_AiName.C_Str(), l_AiName.length);

	Log(Verbose, "Creating MaterialComponent: base='", BaseName, "' idx=", MaterialIndex,
		" name='", l_MaterialName.c_str(), "'", l_WasEmpty ? " (synthesized)" : "");

	OutMaterial = {};
	std::string l_InstanceName = std::string(BaseName) + "." + l_MaterialName + ".MaterialComponent";
	OutMaterial.m_InstanceName = l_InstanceName.c_str();

	auto l_allocation = AssetService::AllocateMaterialAsset(l_MaterialName.c_str(), ObjectLifespan::Scene);
	OutMaterial.m_Asset = l_allocation.m_Handle;

	auto* l_assetData = AssetService::GetMaterialAsset(l_allocation.m_Handle);
	if (!l_assetData)
	{
		Log(Error, "Failed to allocate MaterialAsset for: ", l_MaterialName.c_str());
		return false;
	}

	// TASK-27: recycled allocations may carry stale state from a previous import of the
	// same material name (e.g. re-importing a model). Reset before re-populating.
	if (!l_allocation.m_WasNewlyCreated)
	{
		l_assetData->m_TextureNames.clear();
		l_assetData->m_Attributes = MaterialAttributes{};
	}

	ProcessMaterialProperties(Material, l_assetData);

	ProcessMaterialTextures(Material, BaseName, ModelBaseDir, l_assetData);

	l_assetData->m_Residency = AssetResidency::Resident;

	bool l_Result = AssetService::Save(OutMaterial);

	if (l_Result)
	{
		Log(Success, "Created and saved MaterialComponent: ", l_MaterialName.c_str());
	}
	else
	{
		Log(Error, "Failed to save MaterialComponent: ", l_MaterialName.c_str());
	}

	return l_Result;
}

void AssimpMaterialProcessor::ProcessMaterialProperties(const aiMaterial* material, MaterialAsset* assetData)
{
	aiColor3D l_result;
	aiColor4D l_result4;

	// glTF PBR workflow stores the albedo factor as `baseColorFactor` (exposed
	// via AI_MATKEY_BASE_COLOR, aiColor4D), not AI_MATKEY_COLOR_DIFFUSE (the
	// Phong diffuse). Try the PBR key first; fall back to the legacy Phong key
	// for FBX / older assets. Otherwise the per-material tint is lost (e.g.
	// Sponza curtain_01/02/03 materials all show albedo=1 despite glTF
	// baseColorFactor distinguishing red/green/blue).
	if (material->Get(AI_MATKEY_BASE_COLOR, l_result4) == aiReturn::aiReturn_SUCCESS)
	{
		assetData->m_Attributes.AlbedoR = l_result4.r;
		assetData->m_Attributes.AlbedoG = l_result4.g;
		assetData->m_Attributes.AlbedoB = l_result4.b;
		assetData->m_Attributes.Alpha   = l_result4.a;
	}
	else if (material->Get(AI_MATKEY_COLOR_DIFFUSE, l_result) == aiReturn::aiReturn_SUCCESS)
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

	// PBR metallic/roughness: prefer the glTF-style scalar factors Assimp exposes
	// for real PBR materials. Do NOT translate Phong specular colour or shininess
	// exponent — they are different concepts and the reinterpretation produces
	// spurious metallic-gold cloth, glass-smooth dirt, etc. When the factor
	// isn't present, fall back to a non-metal dielectric default
	// (metallic=0, roughness=0.7); the renderer will still sample texture maps
	// if the material provides them, overriding the scalar fallback per-pixel.
	float l_metallicFactor = 0.0f;
	if (material->Get(AI_MATKEY_METALLIC_FACTOR, l_metallicFactor) == aiReturn::aiReturn_SUCCESS)
		assetData->m_Attributes.Metallic = l_metallicFactor;
	else
		assetData->m_Attributes.Metallic = 0.0f;

	float l_roughnessFactor = 0.0f;
	if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, l_roughnessFactor) == aiReturn::aiReturn_SUCCESS)
		assetData->m_Attributes.Roughness = l_roughnessFactor;
	else
		assetData->m_Attributes.Roughness = 0.7f;

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

void AssimpMaterialProcessor::ProcessMaterialTextures(const aiMaterial* material, const char* baseName, const char* modelBaseDir, MaterialAsset* assetData)
{
	// Slot mapping (matches opaqueGeometryProcessPass.frag and m_TextureIndices_N in common.hlsl):
	//   0 = normal, 1 = albedo, 2 = metallic, 3 = roughness, 4 = AO
	// Pre-size with empty strings so absent textures leave their slot as "" and DrawCallService
	// emits INVALID_TEXTURE_INDEX for that slot, causing the shader to fall back to material attributes.
	constexpr uint32_t kTextureSlotCount = 5;
	assetData->m_TextureNames.assign(kTextureSlotCount, "");

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

			const TextureSlotMapping* l_mapping = nullptr;
			for (const auto& l_candidate : kTextureSlotMappings)
			{
				if (l_candidate.m_AiType == l_aiTextureType)
				{
					l_mapping = &l_candidate;
					break;
				}
			}

			if (!l_mapping)
			{
				Log(Warning, l_AssString.C_Str(), " is unsupported texture type!");
				continue;
			}

			auto l_textureName = AssimpTextureProcessor::CreateTextureComponent(l_localPath, modelBaseDir, TextureSampler::Sampler2D, TextureUsage::Sample, l_mapping->m_IsSRGB, l_mapping->m_SlotIndex, baseName, l_mapping->m_BC4Source);
			if (!l_textureName.empty())
				assetData->m_TextureNames[l_mapping->m_SlotIndex] = l_textureName;
		}
	}
}
