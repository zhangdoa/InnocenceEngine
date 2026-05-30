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

    // Single-char tag per TextureChannelSource ordinal; index with the enum value
    // to label a packed-channel import (e.g. "_chB" for the metallic slot).
    constexpr const char k_ChannelSourceTags[] = "RGBA";

    namespace BCCompression
    {
        // Side effect: input RGBA buffer is freed via STBWrapper::Free.
        void* CompressRGBAToBC(const TextureDesc&   srcDesc,
                               void*                srcRGBA,
                               uint32_t             slotIndex,
                               TextureChannelSource bc4Source,
                               TextureDesc&         outDesc);
    }
}
