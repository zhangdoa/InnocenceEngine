#pragma once
#include "../../engine/component/TransformComponent.h"
#include "../../engine/component/CameraComponent.h"
#include "../../engine/component/InputComponent.h"
#include "../../engine/component/VisibleComponent.h"
#include "../../engine/interface/ILogSystem.h"
extern ILogSystem* g_pLogSystem;

class PlayerCharacter : public BaseComponent
{
public:
	PlayerCharacter() {};
	~PlayerCharacter() {};

	void setup();

	TransformComponent& getTransformComponent();
	VisibleComponent& getVisibleComponent();
	CameraComponent& getCameraComponent();
	InputComponent& getInputComponent();

	std::function<void()> f_moveForward;
	std::function<void()> f_moveBackward;
	std::function<void()> f_moveLeft;
	std::function<void()> f_moveRight;

	std::function<void()> f_allowMove;
	std::function<void()> f_forbidMove;

	std::function<void(double)> f_rotateAroundPositiveYAxis;
	std::function<void(double)> f_rotateAroundRightAxis;

private:
	TransformComponent m_transformComponent;
	VisibleComponent m_visibleComponent;
	InputComponent m_inputComponent;
	CameraComponent m_cameraComponent;

	double m_moveSpeed;
	double m_rotateSpeed;
	bool m_canMove;

	void move(vec4 direction, double length);

	void moveForward();
	void moveBackward();
	void moveLeft();
	void moveRight();

	void allowMove();
	void forbidMove();

	void rotateAroundPositiveYAxis(double offset);
	void rotateAroundRightAxis(double offset);
};

