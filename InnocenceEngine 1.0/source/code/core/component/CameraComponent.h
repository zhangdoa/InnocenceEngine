#pragma once
#include "../interface/IGameEntity.h"

class CameraComponent : public BaseComponent
{
public:
	CameraComponent();
	~CameraComponent();

	void initialize() override;
	void update() override;
	void shutdown() override;

	glm::mat4 getPosMatrix() const;
	glm::mat4 getRotMatrix() const;
	glm::mat4 getProjectionMatrix() const;

private:
	glm::mat4 m_projectionMatrix;
};
