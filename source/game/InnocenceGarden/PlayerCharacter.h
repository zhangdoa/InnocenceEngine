#pragma once
#include "component/TransformComponent.h"
#include "component/CameraComponent.h"
#include "component/InputComponent.h"
#include "component/VisibleComponent.h"
#include "entity/BaseEntity.h"

class PlayerCharacter : public BaseEntity
{
public:
	PlayerCharacter() {};
	~PlayerCharacter() {};

	void setup() override;

	TransformComponent& getTransformComponent();
	CameraComponent& getCameraComponent();
	InputComponent& getInputComponent();
	VisibleComponent& getVisibleComponent();

private:	
	TransformComponent m_transformComponent;
	InputComponent m_inputComponent;
	CameraComponent m_cameraComponent;
	VisibleComponent m_visibleComponent;

	double m_moveSpeed;
	double m_rotateSpeed;

	void move(vec4 direction, double length);

	void moveForward();
	void moveBackward();
	void moveLeft();
	void moveRight();
	std::function<void()> f_moveForward;
	std::function<void()> f_moveBackward;
	std::function<void()> f_moveLeft;
	std::function<void()> f_moveRight;

	void rotateAroundPositiveYAxis(double offset);
	void rotateAroundRightAxis(double offset);

	std::function<void(double)> f_rotateAroundPositiveYAxis;
	std::function<void(double)> f_rotateAroundRightAxis;
};

