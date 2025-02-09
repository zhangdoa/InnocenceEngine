#pragma once
#include "../Common/Object.h"
#include "../Common/MathHelper.h"
#include "TransformComponent.h"
#include "ModelComponent.h"

namespace Inno
{
	struct CollisionPrimitive
	{
		AABB m_AABB = {};
		Sphere m_Sphere = {};
	};

	class CollisionComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 5; };
		static const char* GetTypeName() { return "CollisionComponent"; };

		CollisionPrimitive* m_BottomLevelCollisionPrimitive = 0;
		CollisionPrimitive* m_TopLevelCollisionPrimitive = 0;
		TransformComponent* m_TransformComponent = 0;
		ModelComponent* m_ModelComponent = 0;
		RenderableSet* m_RenderableSet = 0;
		void* m_SimulationProxy = 0;
	};
}