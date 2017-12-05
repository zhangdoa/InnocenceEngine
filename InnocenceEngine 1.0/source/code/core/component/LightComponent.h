#pragma once
#include "../interface/IGameEntity.h"
#include "../data/GraphicData.h"

enum class lightType {DIRECTIONAL, POINT, SPOT};

class LightComponent : public BaseComponent
{
public:
	LightComponent();
	~LightComponent();

	void initialize() override;
	void update() override;
	void shutdown() override;

	const lightType getLightType();
	glm::vec3 getDirection() const;
	float getRadius() const;
	float getConstantFactor() const;
	float getLinearFactor() const;
	float getQuadraticFactor() const;
	glm::vec3 getAmbientColor() const;
	glm::vec3 getDiffuseColor() const;
	glm::vec3 getSpecularColor() const;

	void setlightType(lightType lightType);
	void setDirection(glm::vec3 direction);
	void setRadius(float radius);
	void setConstantFactor(float constantFactor);
	void setLinearFactor(float linearFactor);
	void setQuadraticFactor(float quadraticFactor);
	void setAmbientColor(glm::vec3 ambientColor);
	void setDiffuseColor(glm::vec3 diffuseColor);
	void setSpecularColor(glm::vec3 specularColor);

	void getLightPosMatrix(glm::mat4& lightPosMatrix);
	void getLightRotMatrix(glm::mat4& lightRotMatrix);

	ShadowMapData& getShadowMapData();

private:
	lightType m_lightType = lightType::POINT;
	glm::vec3 m_direction;
	float m_radius;
	float m_constantFactor;
	float m_linearFactor;
	float m_quadraticFactor;
	glm::vec3 m_ambientColor;
	glm::vec3 m_diffuseColor;
	glm::vec3 m_specularColor;
	ShadowMapData m_shadowMapData;
};

