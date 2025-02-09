#pragma once
#include "../Component/CollisionComponent.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
    struct CullingResult
    {
        CollisionComponent* m_CollisionComponent = nullptr;
        VisibilityMask m_VisibilityMask = VisibilityMask::Invalid;
    };
}