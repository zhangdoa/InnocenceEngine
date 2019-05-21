#pragma once
#include "../common/InnoComponent.h"
#include "../common/InnoMath.h"
#include "../common/InnoContainer.h"

class DirectionalLightComponent : public InnoComponent
{
public:
	DirectionalLightComponent() {};
	~DirectionalLightComponent() {};

	// Unit: Lumen (lm)
	float m_luminousFlux = 1.0f;

	// Unitless: use clamped range from 0.0 to 1.0
	// CIE 1931 RGB color space
	vec4 m_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	bool m_drawAABB = false;

	// in World space
	ThreadSafeVector<AABB> m_AABBsInWorldSpace;

	ThreadSafeVector<mat4> m_projectionMatrices;
};
