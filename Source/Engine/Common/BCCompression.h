#pragma once

#include <cstdint>
#include "GraphicsPrimitive.h"

namespace Inno
{
    // Source-channel of the input RGBA8 to extract into a single-channel BC4
    // block. Lets the caller pick the glTF MetallicRoughness packing
    // (R unused, G=roughness, B=metallic) per slot instead of always taking R.
    // R is the safe default for separate single-channel PNGs that STB
    // broadcasts to RGBA on load (R=G=B=L, A=255).
    enum class TextureChannelSource : uint8_t { R = 0, G = 1, B = 2, A = 3 };

    namespace BCCompression
    {
        // Compress an RGBA8 image into the BC format that matches the given
        // texture slot convention (0=normal/BC5, 1=albedo/BC1, 2..=BC4).
        // Returns a Memory::Allocate'd buffer the caller owns; the input
        // RGBA buffer is freed via STBWrapper::Free as a side effect.
        // Updates outDesc with the chosen BC format / Compressed type.
        // Returns nullptr on allocation failure.
        // bc4Source selects which source RGBA channel is packed into BC4;
        // ignored for BC1/BC5 slots.
        void* CompressRGBAToBC(const TextureDesc&   srcDesc,
                               void*                srcRGBA,
                               uint32_t             slotIndex,
                               TextureChannelSource bc4Source,
                               TextureDesc&         outDesc);
    }
}
