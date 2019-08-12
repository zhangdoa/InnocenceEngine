#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMath.h"
#include "../Common/InnoComponent.h"

class PhysicsDataComponent : public InnoComponent
{
public:
	AABB m_AABBLS = {};
	AABB m_AABBWS = {};
	Sphere m_Sphere = {};
	PhysicsDataComponent* m_ParentNode = 0;
	PhysicsDataComponent* m_LeftChildNode = 0;
	PhysicsDataComponent* m_RightChildNode = 0;
	bool m_IsIntermediate = false;
};