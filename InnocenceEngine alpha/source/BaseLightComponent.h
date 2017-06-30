#pragma once
#include "GameObject.h"
#include "Shader.h"

class BaseLightComponent : public GameComponent
{
public:
	BaseLightComponent::BaseLightComponent();
	BaseLightComponent(const Vec3f& color, float intensity);
	~BaseLightComponent();

	const Vec3f& getColor();
	void setColor(const Vec3f& color);
	float getIntensity();
	void setIntensity(float intensity);
	Shader* getShader();
	void setShader(const Shader& shader);

private:
	Vec3f _color;
	float _intensity;
	Shader _shader;
};

