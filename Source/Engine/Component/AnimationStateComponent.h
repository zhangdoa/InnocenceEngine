#pragma once
#include "../Common/FixedSizeString.h"

namespace Inno
{
	struct AnimationStateComponent
	{
		FixedSizeString<128> m_ClipName;
		float m_ElapsedTime = 0.f;
		bool  m_Loop        = true;
		bool  m_Playing     = false;
	};
}
