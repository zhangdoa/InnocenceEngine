#pragma once
#include "../Common/InnoObject.h"
#include "../Common/InnoMathHelper.h"
#include "TransformComponent.h"
#include "VisibleComponent.h"

class PhysicsDataComponent : public InnoComponent
{
public:
	static uint32_t GetTypeID() { return 5; };
	static char* GetTypeName() { return "PhysicsDataComponent"; };

	AABB m_AABBLS = {};
	AABB m_AABBWS = {};
	Sphere m_SphereLS = {};
	Sphere m_SphereWS = {};
	bool m_IsIntermediate = false;
	MeshUsage m_MeshUsage;
	TransformComponent* m_TransformComponent;
	VisibleComponent* m_VisibleComponent;
	MeshMaterialPair* m_MeshMaterialPair;
	void* m_Proxy;
};