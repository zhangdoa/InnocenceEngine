#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"

class PointLightComponent
{
public:
	PointLightComponent() {};
	~PointLightComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	// Unit: Meter (m)
	float m_attenuationRadius = 1.0f;

	// Unit: Lumen (lm)
	float m_luminousFlux = 1.0f;

	// Unitless: use clamped range from 0.0 to 1.0
	// CIE 1931 RGB color space
	vec4 m_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
};

