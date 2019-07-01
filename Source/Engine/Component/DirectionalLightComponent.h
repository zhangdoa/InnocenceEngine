#pragma once
#include "LightComponent.h"
#include "../Common/InnoContainer.h"

class DirectionalLightComponent : public LightComponent
{
public:
	DirectionalLightComponent() {};
	~DirectionalLightComponent() {};

	bool m_drawAABB = false;

	// in World space
	ThreadSafeVector<AABB> m_AABBsInWorldSpace;

	ThreadSafeVector<mat4> m_projectionMatrices;
};
