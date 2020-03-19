#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMathHelper.h"
#include "TransformComponent.h"
#include "VisibleComponent.h"

class PhysicsDataComponent : public InnoComponent
{
public:
	AABB m_AABBLS = {};
	AABB m_AABBWS = {};
	Sphere m_SphereLS = {};
	Sphere m_SphereWS = {};
	bool m_IsIntermediate = false;
	MeshUsage m_MeshUsage;
	TransformComponent* m_TransformComponent;
	VisibleComponent* m_VisibleComponent;
	MeshMaterialPair* m_MeshMaterialPair;
};