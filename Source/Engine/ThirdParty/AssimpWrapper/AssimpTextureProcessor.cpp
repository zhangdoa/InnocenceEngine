#include "AssimpTextureProcessor.h"

#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Services/AssetService.h"
#include "../../Engine.h"

using namespace Inno;

std::string AssimpTextureProcessor::CreateTextureComponent(const char* FileName, const char* ModelBaseDir, TextureSampler Sampler, TextureUsage Usage, bool IsSRGB, uint32_t TextureSlotIndex, const char* BaseName)
{
	// Normalize backslashes to forward slashes (FBX files may use Windows separators).
	std::string l_NormalizedFileName = FileName;
	std::replace(l_NormalizedFileName.begin(), l_NormalizedFileName.end(), '\\', '/');

	auto l_ResolvedPath = std::string(ModelBaseDir) + l_NormalizedFileName;
	auto l_TextureBaseName = g_Engine->Get<IOService>()->getFileName(l_NormalizedFileName.c_str());
	auto l_InstanceName = std::string(BaseName) + "." + l_TextureBaseName;

	return AssetService::ImportTexture(l_ResolvedPath.c_str(), Sampler, Usage, IsSRGB, TextureSlotIndex, l_InstanceName.c_str());
}
