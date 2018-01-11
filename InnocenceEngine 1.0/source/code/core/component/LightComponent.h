#pragma once
#include "../interface/IGameEntity.h"
#include "../data/GraphicData.h"

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
	glm::vec3 getDirection() const;
	float getRadius() const;
	glm::vec3 getColor() const;

	void setlightType(lightType lightType);
	void setDirection(glm::vec3 direction);
	void setRadius(float radius);
	void setColor(glm::vec3 color);

	void getLightPosMatrix(glm::mat4& lightPosMatrix);
	void getLightRotMatrix(glm::mat4& lightRotMatrix);

	//ShadowMapData& getShadowMapData();

private:
	lightType m_lightType = lightType::POINT;
	glm::vec3 m_direction;
	float m_radius;
	float m_constantFactor;
	float m_linearFactor;
	float m_quadraticFactor;
	glm::vec3 m_color;
	//ShadowMapData m_shadowMapData;
};

