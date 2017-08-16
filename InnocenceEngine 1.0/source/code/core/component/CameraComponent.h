#pragma once
#include "../interface/IGameEntity.h"
#include "../manager/InputManager.h"
#include "../manager/WindowManager.h"

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
	int l_mouseRightResult = 0;
	glm::vec2 l_mousePosition;
	int l_keyWResult = 0;
	int l_keySResult = 0;
	int l_keyAResult = 0;
	int l_keyDResult = 0;

	void init() override;
	void update() override;
	void shutdown() override;
};
