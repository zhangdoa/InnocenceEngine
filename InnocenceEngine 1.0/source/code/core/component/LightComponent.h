#pragma once
#include "../interface/IGameEntity.h"

enum class lightType {DIRECTIONAL, POINT, SPOT};

class LightComponent : public BaseComponent
{
public:
	LightComponent();
	~LightComponent();

	const lightType getLightType();
	float getIntensity() const;
	glm::vec3 getAmbientColor() const;
	glm::vec3 getDiffuseColor() const;
	glm::vec3 getSpecularColor() const;

	void setlightType(lightType lightType);
	void setIntensity(float intensity);
	void setAmbientColor(glm::vec3 ambientColor);
	void setDiffuseColor(glm::vec3 diffuseColor);
	void setSpecularColor(glm::vec3 specularColor);

private:
	lightType m_lightType = lightType::POINT;
	float m_intensity;
	glm::vec3 m_ambientColor;
	glm::vec3 m_diffuseColor;
	glm::vec3 m_specularColor;

	glm::vec3 m_direction;

	void initialize() override;
	void update() override;
	void shutdown() override;
};

