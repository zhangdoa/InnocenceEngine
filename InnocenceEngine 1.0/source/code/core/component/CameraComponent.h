#pragma once
#include "../interface/IGameEntity.h"
#include "../manager/WindowManager.h"
#include "../manager/RenderingManager.h"

class CameraComponent : public BaseComponent
{
public:
	CameraComponent();
	~CameraComponent();

	enum moveDirection { FORWARD, BACKWARD, LEFT, RIGHT };

	void getViewProjectionMatrix(glm::mat4& outViewProjectionMatrix) const;
	void getTranslatonMatrix(glm::mat4& outTranslationMatrix) const;
	void getRotationMatrix(glm::mat4& outRotationMatrix) const;
	void getProjectionMatrix(glm::mat4& outProjectionMatrix) const;

	void move(moveDirection moveDirection);

private:
	glm::mat4 m_projectionMatrix;
	float moveSpeed = 0.05f;

	void init() override;
	void update() override;
	void shutdown() override;
};
