#pragma once
#include "../Common/MathHelper.h"

namespace Inno
{
	struct TransformComponent
	{
		static uint32_t GetTypeID() { return 1; };
		static const char* GetTypeName() { return "TransformComponent"; };

		Vec3 m_LocalPos   = {};
		Vec4 m_LocalRot   = Vec4(0.f, 0.f, 0.f, 1.f);  // quaternion XYZW
		Vec3 m_LocalScale = Vec3(1.f, 1.f, 1.f);
	};
}
