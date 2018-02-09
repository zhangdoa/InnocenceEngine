#include "../../main/stdafx.h"
#include "CameraComponent.h"

CameraComponent::CameraComponent()
{
}

CameraComponent::~CameraComponent()
{
}

mat4 CameraComponent::getPosMatrix() const
{
	return getParentActor()->caclWorldPos().mul(-1.0f).toTranslationMartix();
}

mat4 CameraComponent::getRotMatrix() const
{
	// quaternion rotation
	quat conjugateRotQuat = getParentActor()->caclWorldRot();
	conjugateRotQuat.x = -conjugateRotQuat.x;
	conjugateRotQuat.y = -conjugateRotQuat.y;
	conjugateRotQuat.z = -conjugateRotQuat.z;

	return conjugateRotQuat.toRotationMartix();
}

mat4 CameraComponent::getProjectionMatrix() const
{
	return m_projectionMatrix;
}

void CameraComponent::setup()
{
	BaseEntity::setup();
}

void CameraComponent::initialize()
{
	m_projectionMatrix.initializeToPerspectiveMatrix((70.0f / 180.0f) * PI, (16.0f / 9.0f), 0.1f, 1000000.0f);
}

void CameraComponent::update()
{
}

void CameraComponent::shutdown()
{
}

