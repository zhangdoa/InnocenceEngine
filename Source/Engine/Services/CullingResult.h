#pragma once
#include "../Common/EntityID.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
    struct CullingResult
    {
        EntityID m_ModelEntity = INVALID_ENTITY;
        VisibilityMask m_VisibilityMask = VisibilityMask::Invalid;
    };
}