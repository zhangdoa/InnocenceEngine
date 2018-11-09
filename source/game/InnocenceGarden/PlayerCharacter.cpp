#include "PlayerCharacter.h"

void PlayerComponentCollection::setup()
{
	m_transformComponent->m_parentEntity = m_parentEntity;
	m_cameraComponent->m_parentEntity = m_parentEntity;
	m_inputComponent->m_parentEntity = m_parentEntity;
	m_visibleComponent->m_parentEntity = m_parentEntity;

	m_cameraComponent->m_FOVX = 60.0f;
	m_cameraComponent->m_WHRatio = 16.0f / 9.0f;
	m_cameraComponent->m_zNear = 0.1f;
	m_cameraComponent->m_zFar = 200.0f;

	m_moveSpeed = 0.5f;
	m_rotateSpeed = 2.0f;
	m_canMove = false;

	f_moveForward = std::bind(&PlayerComponentCollection::moveForward);
	f_moveBackward = std::bind(&PlayerComponentCollection::moveBackward);
	f_moveLeft = std::bind(&PlayerComponentCollection::moveLeft);
	f_moveRight = std::bind(&PlayerComponentCollection::moveRight);

	f_allowMove = std::bind(&PlayerComponentCollection::allowMove);
	f_forbidMove = std::bind(&PlayerComponentCollection::forbidMove);

	f_rotateAroundPositiveYAxis = std::bind(&PlayerComponentCollection::rotateAroundPositiveYAxis, std::placeholders::_1);
	f_rotateAroundRightAxis = std::bind(&PlayerComponentCollection::rotateAroundRightAxis, std::placeholders::_1);
}

void PlayerComponentCollection::move(vec4 direction, float length)
{
	if (m_canMove)
	{
		m_transformComponent->m_localTransformVector.m_pos = InnoMath::moveTo(m_transformComponent->m_localTransformVector.m_pos, direction, (float)length);
	}
}

void PlayerComponentCollection::moveForward()
{
	move(InnoMath::getDirection(direction::FORWARD, m_transformComponent->m_localTransformVector.m_rot), m_moveSpeed);
}

void PlayerComponentCollection::moveBackward()
{
	move(InnoMath::getDirection(direction::BACKWARD, m_transformComponent->m_localTransformVector.m_rot), m_moveSpeed);
}

void PlayerComponentCollection::moveLeft()
{
	move(InnoMath::getDirection(direction::LEFT, m_transformComponent->m_localTransformVector.m_rot), m_moveSpeed);
}

void PlayerComponentCollection::moveRight()
{
	move(InnoMath::getDirection(direction::RIGHT, m_transformComponent->m_localTransformVector.m_rot), m_moveSpeed);
}

void PlayerComponentCollection::allowMove()
{
	m_canMove = true;
}

void PlayerComponentCollection::forbidMove()
{
	m_canMove = false;
}

void PlayerComponentCollection::rotateAroundPositiveYAxis(float offset)
{
	if (m_canMove)
	{
		m_transformComponent->m_localTransformVector.m_rot = 
			InnoMath::rotateInLocal(
				m_transformComponent->m_localTransformVector.m_rot,
				vec4(0.0f, 1.0f, 0.0f, 0.0f),
				(float)((-offset * m_rotateSpeed) / 180.0f)* PI<float>
			);
	}
}

void PlayerComponentCollection::rotateAroundRightAxis(float offset)
{
	if (m_canMove)
	{
		auto l_right = InnoMath::getDirection(direction::RIGHT, m_transformComponent->m_localTransformVector.m_rot);
		m_transformComponent->m_localTransformVector.m_rot = 
			InnoMath::rotateInLocal(
				m_transformComponent->m_localTransformVector.m_rot,
				l_right,
				(float)((offset * m_rotateSpeed) / 180.0f)* PI<float>);
	}
}
