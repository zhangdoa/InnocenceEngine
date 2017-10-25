#pragma once
#include "../interface/IGameEntity.h"

class LightComponent : public BaseComponent
{
public:
	LightComponent();
	~LightComponent();
	void setIntensity(float intensity);
	void setColor(glm::vec3 color);
	float getIntensity() const;
	glm::vec3 getColor() const;

private:
	float m_intensity = 1.0f;
	glm::vec3 m_color = glm::vec3(1.0f, 1.0f, 1.0f);
	void initialize() override;
	void update() override;
	void shutdown() override;
};

