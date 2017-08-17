#pragma once
#include "../interface/IGameEntity.h"
#include "../manager/CoreManager.h"

class CameraComponent : public BaseComponent
{
public:
	CameraComponent();
	~CameraComponent();

	enum moveDirection { FORWARD, BACKWARD, LEFT, RIGHT };

	glm::mat4 getTranslatonMatrix() const;
	glm::mat4 getRotationMatrix() const;
	glm::mat4 getProjectionMatrix() const;

	void move(moveDirection moveDirection);

private:
	glm::mat4 m_projectionMatrix;
	float moveSpeed = 0.05f;

	void init() override;
	void update() override;
	void shutdown() override;
};
