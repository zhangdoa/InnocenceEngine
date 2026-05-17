#include "BCCompression.h"

#include "Memory.h"
#include "../ThirdParty/STBWrapper/STBWrapper.h"

#include <algorithm>

#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"

namespace Inno
{
    namespace BCCompression
    {
        void* CompressRGBAToBC(const TextureDesc&   srcDesc,
                               void*                srcRGBA,
                               uint32_t             slotIndex,
                               TextureChannelSource bc4Source,
                               TextureDesc&         outDesc)
        {
            uint32_t w = srcDesc.Width;
            uint32_t h = srcDesc.Height;

            // Slot convention:
            //   0 normal map  → BC5 (RG)
            //   1 albedo      → BC1 (RGB; sRGB stays sRGB on the SRV side)
            //   2 metallic    → BC4 (single channel — bc4Source picks which)
            //   3 roughness   → BC4 (single channel — bc4Source picks which)
            //   4 AO          → BC4 (single channel — bc4Source picks which)
            TexturePixelDataFormat bcFormat;
            switch (slotIndex)
            {
            case 0:  bcFormat = TexturePixelDataFormat::BC5; break;
            case 1:  bcFormat = TexturePixelDataFormat::BC1; break;
            default: bcFormat = TexturePixelDataFormat::BC4; break;
            }

            const uint32_t l_BC4SourceOffset = static_cast<uint32_t>(bc4Source);

            uint32_t blockBytes = (bcFormat == TexturePixelDataFormat::BC1 || bcFormat == TexturePixelDataFormat::BC4) ? 8u : 16u;
            uint32_t blocksX    = (w + 3) / 4;
            uint32_t blocksY    = (h + 3) / 4;
            size_t   totalBytes = static_cast<size_t>(blocksX) * blocksY * blockBytes;

            uint8_t* compressed = static_cast<uint8_t*>(Memory::Allocate(totalBytes));
            if (!compressed)
            {
                Memory::Deallocate(srcRGBA);
                return nullptr;
            }

            const uint8_t* src = static_cast<const uint8_t*>(srcRGBA);

            for (uint32_t by = 0; by < blocksY; by++)
            {
                for (uint32_t bx = 0; bx < blocksX; bx++)
                {
                    uint8_t block[64];
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
                        uint8_t l_SingleChannel[16];
                        for (uint32_t i = 0; i < 16; i++) l_SingleChannel[i] = block[i * 4 + l_BC4SourceOffset];
                        stb_compress_bc4_block(dest, l_SingleChannel);
                    }
                    else if (bcFormat == TexturePixelDataFormat::BC5)
                    {
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

            STBWrapper::Free(srcRGBA);
            return compressed;
        }
    }
}
