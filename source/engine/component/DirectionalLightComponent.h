#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"

class DirectionalLightComponent
{
public:
	DirectionalLightComponent() {};
	~DirectionalLightComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity = 0;

	// Unit: Lumen (lm)
	float m_luminousFlux = 1.0f;

	// Unitless: use clamped range from 0.0 to 1.0
	// CIE 1931 RGB color space
	vec4 m_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	bool m_drawAABB = false;
	std::vector<AABB> m_AABBs;

	std::vector<mat4> m_projectionMatrices;
};

