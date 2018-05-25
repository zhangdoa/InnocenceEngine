#include "PlayerCharacter.h"

void PlayerCharacter::setup()
{
	BaseEntity::setup();

	addChildComponent(&m_transformCompoent);
	addChildComponent(&m_cameraComponent);
	addChildComponent(&m_inputComponent);
	addChildComponent(&m_visibleComponent);

	m_cameraComponent.m_FOV = 60.0;
	m_cameraComponent.m_WHRatio = 16.0 / 9.0;
	m_cameraComponent.m_zNear = 0.1;
	m_cameraComponent.m_zFar = 200.0;

	m_moveSpeed = 0.5;
	m_rotateSpeed = 2.0;

	f_moveForward = std::bind(&PlayerCharacter::moveForward, this);
	f_moveBackward = std::bind(&PlayerCharacter::moveBackward, this);
	f_moveLeft = std::bind(&PlayerCharacter::moveLeft, this);
	f_moveRight = std::bind(&PlayerCharacter::moveRight, this);
	
	m_inputComponent.registerKeyboardInputCallback(INNO_KEY_S, &f_moveForward);
	m_inputComponent.registerKeyboardInputCallback(INNO_KEY_W, &f_moveBackward);
	m_inputComponent.registerKeyboardInputCallback(INNO_KEY_A, &f_moveLeft);
	m_inputComponent.registerKeyboardInputCallback(INNO_KEY_D, &f_moveRight);

	f_rotateAroundPositiveYAxis = std::bind(&PlayerCharacter::rotateAroundPositiveYAxis, this, std::placeholders::_1);
	f_rotateAroundRightAxis = std::bind(&PlayerCharacter::rotateAroundRightAxis, this, std::placeholders::_1);
	m_inputComponent.registerMouseInputCallback(0, &f_rotateAroundPositiveYAxis);
	m_inputComponent.registerMouseInputCallback(1, &f_rotateAroundRightAxis);
}

TransformComponent & PlayerCharacter::getTransformComponent()
{
	return m_transformCompoent;
}

CameraComponent & PlayerCharacter::getCameraComponent()
{
	return m_cameraComponent;
}

InputComponent & PlayerCharacter::getInputComponent()
{
	return m_inputComponent;
}

VisibleComponent & PlayerCharacter::getVisibleComponent()
{
	return m_visibleComponent;
}

void PlayerCharacter::move(vec4 direction, double length)
{
	m_transformCompoent.m_transform.setLocalPos(m_transformCompoent.m_transform.getPos() + direction.scale(length));
}

void PlayerCharacter::moveForward()
{
	move(m_transformCompoent.m_transform.getDirection(direction::FORWARD), m_moveSpeed);
}

void PlayerCharacter::moveBackward()
{
	move(m_transformCompoent.m_transform.getDirection(direction::BACKWARD), m_moveSpeed);
}

void PlayerCharacter::moveLeft()
{
	move(m_transformCompoent.m_transform.getDirection(direction::LEFT), m_moveSpeed);
}

void PlayerCharacter::moveRight()
{
	move(m_transformCompoent.m_transform.getDirection(direction::RIGHT), m_moveSpeed);
}

void PlayerCharacter::rotateAroundPositiveYAxis(double offset)
{
	m_transformCompoent.m_transform.rotateInLocal(vec4(0.0, 1.0, 0.0, 0.0), ((-offset * m_rotateSpeed) / 180.0)* PI);
}

void PlayerCharacter::rotateAroundRightAxis(double offset)
{
	m_transformCompoent.m_transform.rotateInLocal(m_transformCompoent.m_transform.getDirection(direction::RIGHT), ((offset * m_rotateSpeed) / 180.0)* PI);
}
