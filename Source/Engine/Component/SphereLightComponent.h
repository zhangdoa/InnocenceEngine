#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMath.h"

class SphereLightComponent : public InnoComponent
{
public:
	SphereLightComponent() {};
	~SphereLightComponent() {};

	// Unit: Meter (m)
	float m_sphereRadius = 1.0f;

	// Unit: Lumen (lm)
	float m_luminousFlux = 1.0f;

	// Unitless: use clamped range from 0.0 to 1.0
	// CIE 1931 RGB color space
	vec4 m_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
};
