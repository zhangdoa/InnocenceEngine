#include "CameraComponent.h"

CameraComponent::CameraComponent()
{
}

CameraComponent::~CameraComponent()
{
}

mat4 CameraComponent::getPosMatrix() const
{
	return getParentActor()->caclWorldPos().mul(-1.0).toTranslationMartix();
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
	m_projectionMatrix.initializeToPerspectiveMatrix((70.0 / 180.0) * PI, (16.0 / 9.0), 0.1, 1000000.0);
}

void CameraComponent::update()
{
}

void CameraComponent::shutdown()
{
}

