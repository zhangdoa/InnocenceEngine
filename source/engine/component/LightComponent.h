#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"

enum class lightType {DIRECTIONAL, POINT, SPOT, RECTANGULAR, SPHERE, DISK, TUBE};

class LightComponent
{
public:
	LightComponent() {};
	~LightComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	lightType m_lightType = lightType::POINT;

	// Unit: Meter (m)
	float m_radius = 1.0f;

	// Unit: Lumen (lm)
	float m_luminousFlux = 1.0f;

	// Unitless: use clamped range from 0.0 to 1.0
	// CIE 1931 RGB color space
	vec4 m_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	bool m_drawAABB = false;

	std::vector<AABB> m_AABBs;
	std::vector<EntityID> m_AABBMeshIDs;

	std::vector<mat4> m_projectionMatrices;
};

