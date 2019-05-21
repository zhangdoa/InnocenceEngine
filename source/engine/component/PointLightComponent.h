#pragma once
#include "../common/InnoComponent.h"
#include "../common/InnoMath.h"

class PointLightComponent : public InnoComponent
{
public:
	PointLightComponent() {};
	~PointLightComponent() {};

	// Unit: Meter (m)
	float m_attenuationRadius = 1.0f;

	// Unit: Lumen (lm)
	float m_luminousFlux = 1.0f;

	// Unitless: use clamped range from 0.0 to 1.0
	// CIE 1931 RGB color space
	vec4 m_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
};
