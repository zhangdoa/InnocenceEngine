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
	TransformComponent m_transformCompoent;
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
};

