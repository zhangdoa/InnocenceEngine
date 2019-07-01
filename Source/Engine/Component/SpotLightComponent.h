#pragma once
#include "LightComponent.h"

class SpotLightComponent : public LightComponent
{
public:
	SpotLightComponent() {};
	~SpotLightComponent() {};

	// Unit: degree in angle
	float m_cutoffAngle = 90.0f;
};
