#include "../../main/stdafx.h"
#include "LightComponent.h"


LightComponent::LightComponent()
{
}


LightComponent::~LightComponent()
{
}

void LightComponent::setIntensity(float intensity)
{
	m_intensity = intensity;
}

void LightComponent::setColor(glm::vec3 color)
{
	m_color = color;
}

float LightComponent::getIntensity() const
{
	return m_intensity;
}

glm::vec3 LightComponent::getColor() const
{
	return m_color;
}

void LightComponent::initialize()
{
}

void LightComponent::update()
{
}

void LightComponent::shutdown()
{
}
