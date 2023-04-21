#pragma once
#include "../Common/Object.h"
#include "../Common/MathHelper.h"
#include "TransformComponent.h"
#include "VisibleComponent.h"

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
		MeshUsage m_MeshUsage;
		TransformComponent* m_TransformComponent;
		VisibleComponent* m_VisibleComponent;
		MeshMaterialPair* m_MeshMaterialPair;
		void* m_Proxy;
	};
}