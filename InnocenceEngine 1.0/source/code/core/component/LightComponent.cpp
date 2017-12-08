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

void LightComponent::setColor(glm::vec3 color)
{
	m_color = color;
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

glm::vec3 LightComponent::getColor() const
{
	return m_color;
}

void LightComponent::initialize()
{
	m_direction = glm::vec3(0.0f, 0.0f, 1.0f);
	m_constantFactor = 1.0f;
	m_linearFactor = 0.14f;
	m_quadraticFactor = 0.07f;
	m_color = glm::vec3(1.0f, 1.0f, 1.0f);
}

void LightComponent::update()
{
	// light volume
	float l_lightMaxIntensity = std::fmaxf(std::fmaxf(m_color.x, m_color.y), m_color.z);
	m_radius = (-m_linearFactor + std::sqrtf(m_linearFactor * m_linearFactor - 4 * m_quadraticFactor * (m_constantFactor - (256.0 / 5.0) * l_lightMaxIntensity))) / (2 * m_quadraticFactor);
}

void LightComponent::shutdown()
{
}
