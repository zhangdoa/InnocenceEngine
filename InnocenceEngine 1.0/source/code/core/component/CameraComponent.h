#pragma once
#include "../interface/IGameEntity.h"
#include "../manager/CoreManager.h"

class CameraComponent : public BaseComponent
{
public:
	CameraComponent();
	~CameraComponent();

	glm::mat4 getTranslatonMatrix() const;
	glm::mat4 getRotationMatrix() const;
	glm::mat4 getProjectionMatrix() const;

private:
	glm::mat4 m_projectionMatrix;

	void init() override;
	void update() override;
	void shutdown() override;
};
