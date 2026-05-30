#include "AssimpTextureProcessor.h"

#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Services/AssetService.h"
#include "../../Engine.h"

using namespace Inno;

std::string AssimpTextureProcessor::CreateTextureComponent(const char* FileName, const char* ModelBaseDir, TextureSampler Sampler, TextureUsage Usage, bool IsSRGB, uint32_t TextureSlotIndex, const char* BaseName, TextureChannelSource BC4Source)
{
	// Normalize backslashes to forward slashes (FBX files may use Windows separators).
	std::string l_NormalizedFileName = FileName;
	std::replace(l_NormalizedFileName.begin(), l_NormalizedFileName.end(), '\\', '/');

	auto l_ResolvedPath = std::string(ModelBaseDir) + l_NormalizedFileName;
	auto l_TextureBaseName = g_Engine->Get<IOService>()->GetFileName(l_NormalizedFileName.c_str());
	auto l_InstanceName = std::string(BaseName) + "." + l_TextureBaseName;

	// glTF KHR_materials_pbrMetallicRoughness packs metallic + roughness into one
	// RGBA texture (G=roughness, B=metallic). Assimp reports it under both
	// aiTextureType_METALNESS and aiTextureType_DIFFUSE_ROUGHNESS, producing two
	// CreateTextureComponent calls with the same source filename but different
	// BC4 channels. Suffix the instance name with the BC4 channel for
	// non-default sources so the two slots land in two distinct .innobin files;
	// without the suffix, AssetService dedup would short-circuit the second
	// call and both material slots would share one BC4 image.
	if (BC4Source != TextureChannelSource::R)
	{
		const char l_ChannelTag = k_ChannelSourceTags[static_cast<uint32_t>(BC4Source)];
		l_InstanceName += "_ch";
		l_InstanceName += l_ChannelTag;
	}

	return AssetService::ImportTexture(l_ResolvedPath.c_str(), Sampler, Usage, IsSRGB, TextureSlotIndex, l_InstanceName.c_str(), BC4Source);
}
