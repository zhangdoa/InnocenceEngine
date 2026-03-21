#pragma once
#include "../Common/STL14.h"

namespace Inno
{
    struct AnimationStateComponent
    {
        std::string m_ClipName;
        float m_ElapsedTime = 0.f;
        bool  m_Loop        = true;
        bool  m_Playing     = false;
    };
}
