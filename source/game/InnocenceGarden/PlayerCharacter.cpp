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

	m_inputComponent.registerInputCallback<PlayerCharacter>(INNO_KEY_S, &PlayerCharacter::moveForward, this);
	m_inputComponent.registerInputCallback<PlayerCharacter>(INNO_KEY_W, &PlayerCharacter::moveBackward, this);
	m_inputComponent.registerInputCallback<PlayerCharacter>(INNO_KEY_A, &PlayerCharacter::moveLeft, this);
	m_inputComponent.registerInputCallback<PlayerCharacter>(INNO_KEY_D, &PlayerCharacter::moveRight, this);
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
