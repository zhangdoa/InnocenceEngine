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

void LightComponent::setModifiedWorldPos(const vec4 & pos)
{
	m_modifiedWorldPos = pos;
}

mat4 LightComponent::getProjectionMatrix() const
{
	auto l_boundMax = m_AABB.m_boundMax;
	auto l_boundMin = m_AABB.m_boundMin;

	mat4 p;
	p.initializeToOrthographicMatrix(l_boundMin.x, l_boundMax.x, l_boundMin.y, l_boundMax.y, l_boundMin.z, l_boundMax.z);

	return p;
}

mat4 LightComponent::getViewMatrix() const
{
	mat4 v;
	v = getInvertRotationMatrix();
	return v;
}

mat4 LightComponent::getInvertTranslationMatrix() const
{
	return getParentEntity()->caclWorldPos().scale(-1.0).toTranslationMatrix();
}

mat4 LightComponent::getInvertRotationMatrix() const
{
	return getParentEntity()->caclWorldRot().quatConjugate().toRotationMatrix();
}

const lightType LightComponent::getLightType() const
{
	return m_lightType;
}

const vec4 LightComponent::getDirection() const
{
	return  getParentEntity()->getTransform()->getDirection(Transform::direction::BACKWARD);
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
