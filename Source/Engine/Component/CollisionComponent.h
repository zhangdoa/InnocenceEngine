#pragma once
#include "../Common/Object.h"
#include "../Common/MathHelper.h"
#include "TransformComponent.h"
#include "ModelComponent.h"

namespace Inno
{
	class CollisionComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 5; };
		static const char* GetTypeName() { return "CollisionComponent"; };

		AABB m_AABB = {};
		Sphere m_Sphere = {};
		void* m_SimulationProxy = 0;
	};
}