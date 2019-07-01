#pragma once
#include "LightComponent.h"

class PointLightComponent : public LightComponent
{
public:
	PointLightComponent() {};
	~PointLightComponent() {};

	// Unit: Meter (m)
	float m_attenuationRadius = 1.0f;
};
