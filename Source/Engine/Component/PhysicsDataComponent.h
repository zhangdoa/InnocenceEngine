#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMathHelper.h"
#include "VisibleComponent.h"

class PhysicsDataComponent : public InnoComponent
{
public:
	AABB m_AABBLS = {};
	AABB m_AABBWS = {};
	Sphere m_SphereLS = {};
	Sphere m_SphereWS = {};
	PhysicsDataComponent* m_ParentNode = 0;
	PhysicsDataComponent* m_LeftChildNode = 0;
	PhysicsDataComponent* m_RightChildNode = 0;
	bool m_IsIntermediate = false;
	VisibleComponent* m_VisibleComponent;
	ModelPair m_ModelPair;
};