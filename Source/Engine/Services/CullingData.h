#pragma once
#include "../Component/PhysicsComponent.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
    struct CullingData
    {
        Mat4 m = Mat4();
        Mat4 m_prev = Mat4();
        Mat4 normalMat = Mat4();
        MeshComponent* mesh = 0;
        MaterialComponent* material = 0;
        MeshUsage meshUsage = MeshUsage::Invalid;
        VisibilityMask visibilityMask = VisibilityMask::Invalid;
        uint64_t UUID = 0;
    };
}