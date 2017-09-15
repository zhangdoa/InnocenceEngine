#pragma once
#include "../interface/IGameEntity.h"
#include "../manager/CoreManager.h"

class InputComponent : public BaseComponent
{
public:
	InputComponent();
	~InputComponent();

private:
	glm::vec2 l_mousePosition;
	int l_mouseRightResult = 0;

	int l_keyWResult = 0;
	int l_keySResult = 0;
	int l_keyAResult = 0;
	int l_keyDResult = 0;

	enum moveDirection { FORWARD, BACKWARD, LEFT, RIGHT };
	float moveSpeed = 0.05f;
	float rotateSpeed = 2.0f;
	void move(moveDirection moveDirection);

	void initialize() override;
	void update() override;
	void shutdown() override;
};

