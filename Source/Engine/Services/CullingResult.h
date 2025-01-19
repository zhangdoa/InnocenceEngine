#pragma once
#include "../Component/PhysicsComponent.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
    struct CullingResult
    {
        PhysicsComponent* m_PhysicsComponent = nullptr;
        VisibilityMask m_VisibilityMask = VisibilityMask::Invalid;
    };
}