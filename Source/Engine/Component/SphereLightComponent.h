#pragma once
#include "LightComponent.h"

class SphereLightComponent : public LightComponent
{
public:
	SphereLightComponent() {};
	~SphereLightComponent() {};

	// Unit: Meter (m)
	float m_sphereRadius = 1.0f;
};
