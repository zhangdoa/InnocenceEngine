#pragma once
#include "../Common/MathHelper.h"

namespace Inno
{
	enum class CollisionShapeType { Box, Sphere, Capsule };

	struct CollisionShapeComponent
	{
		CollisionShapeType m_ShapeType   = CollisionShapeType::Box;
		Vec3               m_HalfExtents = Vec3(0.5f, 0.5f, 0.5f);
		Vec3               m_LocalOffset = {};
	};
}
