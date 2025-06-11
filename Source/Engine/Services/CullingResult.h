#pragma once
#include "../Component/ModelComponent.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
    struct CullingResult
    {
        ModelComponent* m_ModelComponent = nullptr;
        VisibilityMask m_VisibilityMask = VisibilityMask::Invalid;
    };
}