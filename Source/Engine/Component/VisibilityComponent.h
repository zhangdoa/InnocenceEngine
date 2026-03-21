#pragma once
#include "../Common/MathHelper.h"

namespace Inno
{
	struct VisibilityComponent
	{
		AABB m_AABB        = {};
		bool m_Visible     = true;
		bool m_CastShadow  = true;
	};
}
