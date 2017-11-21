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

void LightComponent::setDirection(glm::vec3 direction)
{
	m_direction = direction;
}

void LightComponent::setRadius(float radius)
{
	m_radius = radius;
}

void LightComponent::setConstantFactor(float constantFactor)
{
	m_constantFactor = constantFactor;
}

void LightComponent::setLinearFactor(float linearFactor)
{
	m_linearFactor = linearFactor;
}

void LightComponent::setQuadraticFactor(float quadraticFactor)
{
	m_quadraticFactor = quadraticFactor;
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

void LightComponent::getLightPosMatrix(glm::mat4 & lightPosMatrix)
{
	lightPosMatrix = getParentActor()->caclWorldPosMatrix();
}

void LightComponent::getLightRotMatrix(glm::mat4 & lightRotMatrix)
{
	lightRotMatrix = getParentActor()->caclWorldRotMatrix();
}

ShadowMapData & LightComponent::getShadowMapData()
{
	return m_shadowMapData;
}


const lightType LightComponent::getLightType()
{
	return m_lightType;
}

glm::vec3 LightComponent::getDirection() const
{
	return m_direction;
}

float LightComponent::getRadius() const
{
	return m_radius;
}

float LightComponent::getConstantFactor() const
{
	return m_constantFactor;
}

float LightComponent::getLinearFactor() const
{
	return m_linearFactor;
}

float LightComponent::getQuadraticFactor() const
{
	return m_quadraticFactor;
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
	m_direction = glm::vec3(0.0f, 0.0f, 1.0f);
	m_constantFactor = 1.0f;
	m_linearFactor = 0.14f;
	m_quadraticFactor = 0.07f;
	m_ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
	m_diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	m_specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
}

void LightComponent::update()
{
	// light volume
	float l_lightMaxIntensity = std::fmaxf(std::fmaxf(m_diffuseColor.x, m_diffuseColor.y), m_diffuseColor.z);
	m_radius = (-m_linearFactor + std::sqrtf(m_linearFactor * m_linearFactor - 4 * m_quadraticFactor * (m_constantFactor - (256.0 / 5.0) * l_lightMaxIntensity))) / (2 * m_quadraticFactor);
}

void LightComponent::shutdown()
{
}
