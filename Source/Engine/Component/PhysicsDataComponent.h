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
	bool m_IsIntermediate = false;
	VisibleComponent* m_VisibleComponent;
	ModelPair m_ModelPair;
};