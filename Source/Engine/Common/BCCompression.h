#pragma once

#include <cstdint>
#include "GraphicsPrimitive.h"

namespace Inno
{
    namespace BCCompression
    {
        // Compress an RGBA8 image into the BC format that matches the given
        // texture slot convention (0=normal/BC5, 1=albedo/BC1, 2..=BC4).
        // Returns a Memory::Allocate'd buffer the caller owns; the input
        // RGBA buffer is freed via STBWrapper::Free as a side effect.
        // Updates outDesc with the chosen BC format / Compressed type.
        // Returns nullptr on allocation failure.
        void* CompressRGBAToBC(const TextureDesc& srcDesc,
                               void*              srcRGBA,
                               uint32_t           slotIndex,
                               TextureDesc&       outDesc);
    }
}
