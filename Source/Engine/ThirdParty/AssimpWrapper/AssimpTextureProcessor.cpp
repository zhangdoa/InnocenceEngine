#include "AssimpTextureProcessor.h"

#include "../../Common/LogService.h"
#include "../../Common/IOService.h"
#include "../../Common/Memory.h"
#include "../../Services/AssetService.h"
#include "../../ThirdParty/STBWrapper/STBWrapper.h"
#include "../../Engine.h"

#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"

using namespace Inno;

namespace AssimpTextureProcessorNS
{
	// Returns malloc'd BC-compressed data and updates desc in-place.
	// Frees the input RGBA data via stbi_image_free convention (direct free).
	// src is RGBA UByte, 4 bytes per pixel.
	static void* CompressToBC(const TextureDesc& srcDesc, void* srcRGBA, uint32_t slotIndex, TextureDesc& outDesc)
	{
		uint32_t w = srcDesc.Width;
		uint32_t h = srcDesc.Height;

		// Choose BC format per slot:
		//   0 = normal map  → BC5 (RG)
		//   1 = albedo      → BC1 (RGB, sRGB)
		//   2 = metallic    → BC4 (R)
		//   3 = roughness   → BC4 (R)
		//   4 = AO          → BC4 (R)
		TexturePixelDataFormat bcFormat;
		switch (slotIndex)
		{
		case 0:  bcFormat = TexturePixelDataFormat::BC5; break;
		case 1:  bcFormat = TexturePixelDataFormat::BC1; break;
		default: bcFormat = TexturePixelDataFormat::BC4; break;
		}

		uint32_t blockBytes = (bcFormat == TexturePixelDataFormat::BC1 || bcFormat == TexturePixelDataFormat::BC4) ? 8u : 16u;
		uint32_t blocksX    = (w + 3) / 4;
		uint32_t blocksY    = (h + 3) / 4;
		size_t   totalBytes = static_cast<size_t>(blocksX) * blocksY * blockBytes;

		uint8_t* compressed = static_cast<uint8_t*>(Inno::Memory::Allocate(totalBytes));
		if (!compressed)
		{
			Inno::Memory::Deallocate(srcRGBA);
			return nullptr;
		}

		const uint8_t* src = static_cast<const uint8_t*>(srcRGBA);

		for (uint32_t by = 0; by < blocksY; by++)
		{
			for (uint32_t bx = 0; bx < blocksX; bx++)
			{
				// Gather 4×4 block from source (pad at edges)
				uint8_t block[64]; // max 16 pixels × 4 bytes (RGBA)
				for (uint32_t py = 0; py < 4; py++)
				{
					for (uint32_t px = 0; px < 4; px++)
					{
						uint32_t sx = std::min(bx * 4 + px, w - 1);
						uint32_t sy = std::min(by * 4 + py, h - 1);
						const uint8_t* pixel = src + (sy * w + sx) * 4;
						block[(py * 4 + px) * 4 + 0] = pixel[0];
						block[(py * 4 + px) * 4 + 1] = pixel[1];
						block[(py * 4 + px) * 4 + 2] = pixel[2];
						block[(py * 4 + px) * 4 + 3] = pixel[3];
					}
				}

				uint8_t* dest = compressed + (by * blocksX + bx) * blockBytes;

				if (bcFormat == TexturePixelDataFormat::BC1)
				{
					stb_compress_dxt_block(dest, block, 0, STB_DXT_NORMAL);
				}
				else if (bcFormat == TexturePixelDataFormat::BC4)
				{
					// Extract R channel: 16 bytes, 1 byte per pixel
					uint8_t r[16];
					for (uint32_t i = 0; i < 16; i++) r[i] = block[i * 4];
					stb_compress_bc4_block(dest, r);
				}
				else if (bcFormat == TexturePixelDataFormat::BC5)
				{
					// Extract RG channels: 32 bytes, 2 bytes per pixel
					uint8_t rg[32];
					for (uint32_t i = 0; i < 16; i++) { rg[i * 2] = block[i * 4]; rg[i * 2 + 1] = block[i * 4 + 1]; }
					stb_compress_bc5_block(dest, rg);
				}
			}
		}

		outDesc = srcDesc;
		outDesc.PixelDataFormat = bcFormat;
		outDesc.PixelDataType   = TexturePixelDataType::Compressed;
		outDesc.MipLevels       = 1;

		Inno::Memory::Deallocate(srcRGBA);
		return compressed;
	}
}

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

	void* l_RawData = STBWrapper::Load(l_ResolvedPath.c_str(), l_Texture);
	if (!l_RawData)
	{
		Log(Error, "Failed to load texture data: ", l_ResolvedPath.c_str());
		return {};
	}

	TextureDesc l_CompressedDesc = {};
	void* l_TextureData = AssimpTextureProcessorNS::CompressToBC(l_Texture.m_TextureDesc, l_RawData, TextureSlotIndex, l_CompressedDesc);
	if (!l_TextureData)
	{
		Log(Error, "Failed to BC-compress texture: ", l_ResolvedPath.c_str());
		return {};
	}
	l_Texture.m_TextureDesc = l_CompressedDesc;

	bool l_Result = AssetService::Save(l_Texture, l_TextureData);

	if (l_Result)
	{
		Log(Success, "Created and saved TextureComponent: ", l_ResolvedPath.c_str());
		return l_Name;
	}

	Log(Error, "Failed to save TextureComponent: ", l_ResolvedPath.c_str());
	return {};
}
