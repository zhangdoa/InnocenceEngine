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

void LightComponent::setRadius(double radius)
{
	m_radius = radius;
}

void LightComponent::setColor(const vec3& color)
{
	m_color = color;
}

void LightComponent::getLightPosMatrix(mat4 & lightPosMatrix)
{
	lightPosMatrix = getParentEntity()->caclWorldPosMatrix();
}

void LightComponent::getLightRotMatrix(mat4 & lightRotMatrix)
{
	lightRotMatrix = getParentEntity()->caclWorldRotMatrix();
}

const lightType LightComponent::getLightType()
{
	return m_lightType;
}

vec3 LightComponent::getDirection() const
{
	return  getParentEntity()->getTransform()->getDirection(Transform::direction::FORWARD);
}

double LightComponent::getRadius() const
{
	double l_lightMaxIntensity = std::fmax(std::fmax(m_color.x, m_color.y), m_color.z);
	return 	 (-m_linearFactor + std::sqrt(m_linearFactor * m_linearFactor - 4.0 * m_quadraticFactor * (m_constantFactor - (256.0 / 5.0) * l_lightMaxIntensity))) / (2.0 * m_quadraticFactor);
}

vec3 LightComponent::getColor() const
{
	return m_color;
}

void LightComponent::setup()
{
}

void LightComponent::initialize()
{
	m_direction = vec3(0.0, 0.0, 1.0);
	m_constantFactor = 1.0;
	m_linearFactor = 0.14;
	m_quadraticFactor = 0.07;
	m_color = vec3(1.0, 1.0, 1.0);
}

void LightComponent::update()
{
}

void LightComponent::shutdown()
{
}
