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

	const lightType getLightType() const;
	const vec4 getDirection() const;
	const double getRadius() const;
	const vec4 getColor() const;

	void setlightType(const lightType lightType);
	void setRadius(const double radius);
	void setColor(const vec4& color);
	void setModifiedWorldPos(const vec4& pos);
	mat4 getProjectionMatrix() const;
	mat4 getViewMatrix() const;
	mat4 getInvertTranslationMatrix() const;
	mat4 getInvertRotationMatrix() const;

	bool m_drawAABB = false;
	meshID m_AABBMeshID;
	AABB m_AABB;
	vec4 m_modifiedWorldPos;
private:
	lightType m_lightType = lightType::POINT;
	vec4 m_direction;	

	double m_radius;
	double m_constantFactor;
	double m_linearFactor;
	double m_quadraticFactor;
	vec4 m_color;
};

