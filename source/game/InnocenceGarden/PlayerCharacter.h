#pragma once
#include "../../engine/component/TransformComponent.h"
#include "../../engine/component/CameraComponent.h"
#include "../../engine/component/InputComponent.h"
#include "../../engine/component/VisibleComponent.h"

class PlayerComponent
{
public:
	PlayerComponent() {};
	~PlayerComponent() {};

	void setup();

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	TransformComponent* m_transformComponent;
	VisibleComponent* m_visibleComponent;
	InputComponent* m_inputComponent;
	CameraComponent* m_cameraComponent;

	std::function<void()> f_moveForward;
	std::function<void()> f_moveBackward;
	std::function<void()> f_moveLeft;
	std::function<void()> f_moveRight;

	std::function<void()> f_allowMove;
	std::function<void()> f_forbidMove;

	std::function<void(float)> f_rotateAroundPositiveYAxis;
	std::function<void(float)> f_rotateAroundRightAxis;

	float m_moveSpeed = 0;
	float m_rotateSpeed = 0;
	bool m_canMove = false;

private:
	void move(vec4 direction, float length);

	void moveForward();
	void moveBackward();
	void moveLeft();
	void moveRight();

	void allowMove();
	void forbidMove();

	void rotateAroundPositiveYAxis(float offset);
	void rotateAroundRightAxis(float offset);
};

