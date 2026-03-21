#pragma once
#include "../Common/MathHelper.h"

namespace Inno
{
    struct TransformComponent
    {
        Vec3 m_LocalPos   = {};
        Vec4 m_LocalRot   = Vec4(0.f, 0.f, 0.f, 1.f);  // quaternion XYZW
        Vec3 m_LocalScale = Vec3(1.f, 1.f, 1.f);
        Mat4 m_WorldMatrix = {};
        bool m_Dirty = true;
    };
}
