#pragma once
#include "BaseComponent.h"
#include "entity/BaseGraphicPrimitive.h"

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
	vec3 getDirection() const;
	double getRadius() const;
	vec3 getColor() const;

	void setlightType(lightType lightType);
	void setDirection(vec3 direction);
	void setRadius(double radius);
	void setColor(const vec3& color);

	void getLightPosMatrix(mat4 & lightPosMatrix);
	void getLightRotMatrix(mat4 & lightRotMatrix);

	//ShadowMapData& getShadowMapData();

private:
	lightType m_lightType = lightType::POINT;
	vec3 m_direction;
	double m_radius;
	double m_constantFactor;
	double m_linearFactor;
	double m_quadraticFactor;
	vec3 m_color;
	//ShadowMapData m_shadowMapData;
};

