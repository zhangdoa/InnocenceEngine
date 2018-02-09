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

void LightComponent::setDirection(vec3 direction)
{
	m_direction = direction;
}

void LightComponent::setRadius(float radius)
{
	m_radius = radius;
}

void LightComponent::setColor(const vec3& color)
{
	m_color = color;
}


void LightComponent::getLightPosMatrix(mat4 & lightPosMatrix)
{
	lightPosMatrix = getParentActor()->caclWorldPosMatrix();
}

void LightComponent::getLightRotMatrix(mat4 & lightRotMatrix)
{
	lightRotMatrix = getParentActor()->caclWorldRotMatrix();
}

const lightType LightComponent::getLightType()
{
	return m_lightType;
}

vec3 LightComponent::getDirection() const
{
	return m_direction;
}

float LightComponent::getRadius() const
{
	float l_lightMaxIntensity = std::fmaxf(std::fmaxf(m_color.x, m_color.y), m_color.z);
	return 	 (-m_linearFactor + std::sqrtf(m_linearFactor * m_linearFactor - 4.0f * m_quadraticFactor * (m_constantFactor - (256.0f / 5.0f) * l_lightMaxIntensity))) / (2.0f * m_quadraticFactor);
}

vec3 LightComponent::getColor() const
{
	return m_color;
}

void LightComponent::setup()
{
	BaseEntity::setup();
}

void LightComponent::initialize()
{
	m_direction = vec3(0.0f, 0.0f, 1.0f);
	m_constantFactor = 1.0f;
	m_linearFactor = 0.14f;
	m_quadraticFactor = 0.07f;
	m_color = vec3(1.0f, 1.0f, 1.0f);
}

void LightComponent::update()
{
}

void LightComponent::shutdown()
{
}
