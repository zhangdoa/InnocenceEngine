#pragma once
#include "../Common/MathHelper.h"

namespace Inno
{
    struct WorldTransformComponent
    {
        Mat4 m_WorldMatrix         = {};
        Mat4 m_WorldRotationMatrix = {};
        bool m_Dirty               = true;
    };
}
