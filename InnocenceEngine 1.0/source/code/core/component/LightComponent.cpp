#include "../../main/stdafx.h"
#include "LightComponent.h"


LightComponent::LightComponent()
{
}


LightComponent::~LightComponent()
{
}

void LightComponent::setlightType(lightType lightType)
{
	m_lightType = lightType;
}

void LightComponent::setIntensity(float intensity)
{
	m_intensity = intensity;
}

void LightComponent::setAmbientColor(glm::vec3 ambientColor)
{
	m_ambientColor = ambientColor;
}

void LightComponent::setDiffuseColor(glm::vec3 diffuseColor)
{
	m_diffuseColor = diffuseColor;
}

void LightComponent::setSpecularColor(glm::vec3 specularColor)
{
	m_specularColor = specularColor;
}


const lightType LightComponent::getLightType()
{
	return m_lightType;
}

float LightComponent::getIntensity() const
{
	return m_intensity;
}

glm::vec3 LightComponent::getAmbientColor() const
{
	return m_ambientColor;
}

glm::vec3 LightComponent::getDiffuseColor() const
{
	return m_diffuseColor;
}

glm::vec3 LightComponent::getSpecularColor() const
{
	return m_specularColor;
}


void LightComponent::initialize()
{
	m_intensity = 1.0f;
	m_ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
	m_diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	m_specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
}

void LightComponent::update()
{
}

void LightComponent::shutdown()
{
}
