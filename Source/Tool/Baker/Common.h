#pragma once
#include "../../Engine/Common/GPUDataStructure.h"

namespace Inno
{
    struct BrickCache
    {
        Vec4 pos;
        std::vector<Surfel> surfelCaches;
    };

    struct BrickCacheSummary
    {
        Vec4 pos;
        size_t fileIndex;
        size_t fileSize;
    };
}