#pragma once
#include "BaseComponent.h"
#include "entity/innoMath.h"

enum class lightType {DIRECTIONAL, POINT, SPOT};

class LightComponent : public BaseComponent
{
public:
	LightComponent();
	~LightComponent();


	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	const lightType getLightType();
	vec4 getDirection() const;
	double getRadius() const;
	vec4 getColor() const;

	void setlightType(lightType lightType);
	void setRadius(double radius);
	void setColor(const vec4& color);

	mat4 getLightPosMatrix();
	mat4 getLightRotMatrix();

	bool m_drawAABB = false;
	meshID m_AABBMeshID;
	AABB m_AABB;

private:
	lightType m_lightType = lightType::POINT;
	vec4 m_direction;
	double m_radius;
	double m_constantFactor;
	double m_linearFactor;
	double m_quadraticFactor;
	vec4 m_color;
};

