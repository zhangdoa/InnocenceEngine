#pragma once
#include "BaseComponent.h"
#include "entity/InnoMath.h"

enum class lightType {DIRECTIONAL, POINT, SPOT};

class LightComponent : public BaseComponent
{
public:
	LightComponent();
	~LightComponent();

	void setup() override;
	void initialize() override;
	void shutdown() override;

	mat4 getProjectionMatrix(unsigned int cascadedLevel) const;

	lightType m_lightType = lightType::POINT;
	vec4 m_direction;

	double m_constantFactor;
	double m_linearFactor;
	double m_quadraticFactor;
	double m_radius;

	vec4 m_color;

	bool m_drawAABB = false;

	std::vector<AABB> m_AABBs;
	std::vector<meshID> m_AABBMeshIDs;
};

