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

void LightComponent::setColor(const vec4& color)
{
	m_color = color;
}

mat4 LightComponent::getProjectionMatrix() const
{
	return mat4();
}

mat4 LightComponent::getInvertTranslationMatrix() const
{
	return getParentEntity()->caclWorldPos().scale(-1.0).toTranslationMatrix();
}

mat4 LightComponent::getInvertRotationMatrix() const
{
	// quaternion rotation
	vec4 conjugateRotQuat = getParentEntity()->caclWorldRot();
	conjugateRotQuat.x = -conjugateRotQuat.x;
	conjugateRotQuat.y = -conjugateRotQuat.y;
	conjugateRotQuat.z = -conjugateRotQuat.z;

	return conjugateRotQuat.toRotationMatrix();
}

const lightType LightComponent::getLightType() const
{
	return m_lightType;
}

const vec4 LightComponent::getDirection() const
{
	return  getParentEntity()->getTransform()->getDirection(Transform::direction::FORWARD);
}

const double LightComponent::getRadius() const
{
	double l_lightMaxIntensity = std::fmax(std::fmax(m_color.x, m_color.y), m_color.z);
	return 	 (-m_linearFactor + std::sqrt(m_linearFactor * m_linearFactor - 4.0 * m_quadraticFactor * (m_constantFactor - (256.0 / 5.0) * l_lightMaxIntensity))) / (2.0 * m_quadraticFactor);
}

const vec4 LightComponent::getColor() const
{
	return m_color;
}

void LightComponent::setup()
{
}

void LightComponent::initialize()
{
	m_direction = vec4(0.0, 0.0, 1.0, 0.0);
	m_constantFactor = 1.0;
	m_linearFactor = 0.14;
	m_quadraticFactor = 0.07;
	m_color = vec4(1.0, 1.0, 1.0, 1.0);
}

void LightComponent::update()
{
}

void LightComponent::shutdown()
{
}
