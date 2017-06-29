#include "BaseLightComponent.h"



BaseLightComponent::BaseLightComponent()
{
	this->_color = Vec3f(0.0f, 0.0f, 0.0f);
	this->_intensity = 1.0f;
	this->_shader = Shader();
	getParent()->getRenderingEngine()->addLight(this);
}


BaseLightComponent::BaseLightComponent(const Vec3f& color, float intensity)
{
	this->_color = color;
	this->_intensity = intensity;
	this->_shader = Shader();
	getParent()->getRenderingEngine()->addLight(this);
}

BaseLightComponent::~BaseLightComponent()
{
}

const Vec3f& BaseLightComponent::getColor()
{
	return _color;
}

void BaseLightComponent::setColor(const Vec3f & color)
{
}

float BaseLightComponent::getIntensity()
{
	return _intensity;
}

void BaseLightComponent::setIntensity(float intensity)
{
	_intensity = intensity;
}

Shader* BaseLightComponent::getShader()
{
	return &_shader;
}

void BaseLightComponent::setShader(const Shader & shader)
{
	_shader = shader;
}