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

	const lightType getLightType() const;
	const vec4 getDirection() const;
	const double getRadius() const;
	const vec4 getColor() const;

	void setlightType(const lightType lightType);
	void setRadius(const double radius);
	void setColor(const vec4& color);

	mat4 getProjectionMatrix(unsigned int cascadedLevel) const;

	bool m_drawAABB = false;
	meshID m_AABBMeshID;
	AABB m_AABB;

	std::vector<AABB> m_AABBs;
	std::vector<meshID> m_AABBMeshIDs;

private:
	lightType m_lightType = lightType::POINT;
	vec4 m_direction;	

	double m_radius;
	double m_constantFactor;
	double m_linearFactor;
	double m_quadraticFactor;
	vec4 m_color;
};

