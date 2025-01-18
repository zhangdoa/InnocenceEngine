#pragma once
#include "../Common/Object.h"
#include "../Common/MathHelper.h"
#include "TransformComponent.h"
#include "ModelComponent.h"

namespace Inno
{
	class PhysicsComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 5; };
		static const char* GetTypeName() { return "PhysicsComponent"; };

		AABB m_AABBLS = {};
		AABB m_AABBWS = {};
		Sphere m_SphereLS = {};
		Sphere m_SphereWS = {};
		TransformComponent* m_TransformComponent;
		ModelComponent* m_ModelComponent;
		RenderableSet* m_RenderableSet;
		void* m_Proxy;
	};
}