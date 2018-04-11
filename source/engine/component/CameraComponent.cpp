#include "CameraComponent.h"

CameraComponent::CameraComponent()
{
}

CameraComponent::~CameraComponent()
{
}

mat4 CameraComponent::getPosMatrix() const
{
	return getParentEntity()->caclWorldPos().scale(-1.0).toTranslationMartix();
}

mat4 CameraComponent::getRotMatrix() const
{
	return getParentEntity()->caclWorldRot().quatConjugate().toRotationMartix();
}

mat4 CameraComponent::getProjectionMatrix() const
{
	return m_projectionMatrix;
}

std::vector<Vertex>* CameraComponent::getFrustumCorners()
{
	return &m_frustumCorners;
}

void CameraComponent::setup()
{
	m_projectionMatrix.initializeToPerspectiveMatrix((60.0 / 180.0) * PI, (16.0 / 9.0), 0.01, 10000.0);
	m_rayOfEye.m_origin = getParentEntity()->caclWorldPos();
	m_rayOfEye.m_direction = getParentEntity()->getTransform()->getDirection(Transform::direction::FORWARD);
	// @TODO: cacl m_frustumCorners
}

void CameraComponent::initialize()
{
}

void CameraComponent::update()
{
	m_rayOfEye.m_origin = getParentEntity()->caclWorldPos();
	m_rayOfEye.m_direction = getParentEntity()->getTransform()->getDirection(Transform::direction::FORWARD);
}

void CameraComponent::shutdown()
{
}

