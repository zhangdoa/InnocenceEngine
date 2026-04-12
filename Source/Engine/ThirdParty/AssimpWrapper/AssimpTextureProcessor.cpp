#include "AssimpTextureProcessor.h"

#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Services/AssetService.h"
#include "../../ThirdParty/STBWrapper/STBWrapper.h"
#include "../../Engine.h"

using namespace Inno;

std::string AssimpTextureProcessor::CreateTextureComponent(const char* FileName, const char* ModelBaseDir, TextureSampler Sampler, TextureUsage Usage, bool IsSRGB, uint32_t TextureSlotIndex, const char* BaseName)
{
	// Normalize backslashes to forward slashes (FBX files may use Windows separators).
	std::string l_NormalizedFileName = FileName;
	std::replace(l_NormalizedFileName.begin(), l_NormalizedFileName.end(), '\\', '/');

	// Resolve the texture path relative to the model's directory.
	// FileName is relative to the model file; ModelBaseDir is the model's directory
	// relative to the engine working directory.
	auto l_ResolvedPath = std::string(ModelBaseDir) + l_NormalizedFileName;

	Log(Verbose, "Creating TextureComponent for: ", l_ResolvedPath.c_str());

	if (!g_Engine->Get<IOService>()->isFileExist(l_ResolvedPath.c_str()))
	{
		Log(Warning, "Texture file not found: ", l_ResolvedPath.c_str());
		return {};
	}

	// Use only the stem (base filename without extension and without directory) so the
	// component name contains no path separators. FixedSizeString overwrites its last
	// character with '\0'; all component names therefore carry a trailing '/' as the
	// sacrificial character. The returned name (stored in MaterialComponent JSON) must
	// NOT include the trailing '/' — only the m_InstanceName assignment uses it.
	auto l_TextureBaseName = g_Engine->Get<IOService>()->getFileName(l_NormalizedFileName.c_str());
	auto l_Name = std::string(BaseName) + "." + l_TextureBaseName;
	auto l_InstanceName = l_Name + "/";

	TextureComponent l_Texture = {};
	l_Texture.m_InstanceName = l_InstanceName.c_str();
	l_Texture.m_TextureDesc.Sampler = Sampler;
	l_Texture.m_TextureDesc.Usage = Usage;
	l_Texture.m_TextureDesc.IsSRGB = IsSRGB;

	void* l_TextureData = STBWrapper::Load(l_ResolvedPath.c_str(), l_Texture);
	if (!l_TextureData)
	{
		Log(Error, "Failed to load texture data: ", l_ResolvedPath.c_str());
		return {};
	}

	bool l_Result = AssetService::Save(l_Texture, l_TextureData);

	if (l_Result)
	{
		Log(Success, "Created and saved TextureComponent: ", l_ResolvedPath.c_str());
		return l_Name;
	}

	Log(Error, "Failed to save TextureComponent: ", l_ResolvedPath.c_str());
	return {};
}
