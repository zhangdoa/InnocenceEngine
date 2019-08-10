#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMath.h"

class LightComponent : public InnoComponent
{
public:
	// Unitless: use clamped range from 0.0 to 1.0
	// CIE 1931 RGB color space
	vec4 m_RGBColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Unit: Kelvin (K)
	float m_ColorTemperature = 5780.0f;

	// Unit: Lumen (lm)
	float m_LuminousFlux = 1.0f;

	bool m_UseColorTemperature = true;
};
