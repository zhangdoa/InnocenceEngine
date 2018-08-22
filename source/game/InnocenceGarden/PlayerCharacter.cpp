#include "PlayerCharacter.h"

void PlayerCharacter::setup()
{
	m_transformComponent.setParentEntity(m_parentEntity);
	m_cameraComponent.setParentEntity(m_parentEntity);
	m_inputComponent.setParentEntity(m_parentEntity);
	m_visibleComponent.setParentEntity(m_parentEntity);

	m_cameraComponent.m_FOV = 60.0;
	m_cameraComponent.m_WHRatio = 16.0 / 9.0;
	m_cameraComponent.m_zNear = 0.1;
	m_cameraComponent.m_zFar = 20000.0;

	m_moveSpeed = 0.5;
	m_rotateSpeed = 2.0;
	m_canMove = false;

	f_moveForward = std::bind(&PlayerCharacter::moveForward, this);
	f_moveBackward = std::bind(&PlayerCharacter::moveBackward, this);
	f_moveLeft = std::bind(&PlayerCharacter::moveLeft, this);
	f_moveRight = std::bind(&PlayerCharacter::moveRight, this);

	f_allowMove = std::bind(&PlayerCharacter::allowMove, this);
	f_forbidMove = std::bind(&PlayerCharacter::forbidMove, this);

	f_rotateAroundPositiveYAxis = std::bind(&PlayerCharacter::rotateAroundPositiveYAxis, this, std::placeholders::_1);
	f_rotateAroundRightAxis = std::bind(&PlayerCharacter::rotateAroundRightAxis, this, std::placeholders::_1);
}

TransformComponent & PlayerCharacter::getTransformComponent()
{
	return m_transformComponent;
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
	if (m_canMove)
	{
		m_transformComponent.m_transform.setLocalPos(m_transformComponent.m_transform.getPos() + direction.scale(length));
	}
}

void PlayerCharacter::moveForward()
{
	move(m_transformComponent.m_transform.getDirection(direction::FORWARD), m_moveSpeed);
}

void PlayerCharacter::moveBackward()
{
	move(m_transformComponent.m_transform.getDirection(direction::BACKWARD), m_moveSpeed);
}

void PlayerCharacter::moveLeft()
{
	move(m_transformComponent.m_transform.getDirection(direction::LEFT), m_moveSpeed);
}

void PlayerCharacter::moveRight()
{
	move(m_transformComponent.m_transform.getDirection(direction::RIGHT), m_moveSpeed);
}

void PlayerCharacter::allowMove()
{
	m_canMove = true;
}

void PlayerCharacter::forbidMove()
{
	m_canMove = false;
}

void PlayerCharacter::rotateAroundPositiveYAxis(double offset)
{
	if (m_canMove)
	{
		m_transformComponent.m_transform.rotateInLocal(vec4(0.0, 1.0, 0.0, 0.0), ((-offset * m_rotateSpeed) / 180.0)* PI);
	}
}

void PlayerCharacter::rotateAroundRightAxis(double offset)
{
	if (m_canMove)
	{
		m_transformComponent.m_transform.rotateInLocal(m_transformComponent.m_transform.getDirection(direction::RIGHT), ((offset * m_rotateSpeed) / 180.0)* PI);
	}
}
