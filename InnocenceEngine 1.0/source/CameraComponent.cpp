#include "stdafx.h"
#include "CameraComponent.h"

CameraData::CameraData()
{
}

CameraData::~CameraData()
{
}

void CameraData::addCameraData(float fov, float aspectRatio, float zNear, float zFar)
{
	m_projectionMatrix.initPerspective(fov, aspectRatio, zNear, zFar);
}

Mat4f * CameraData::getProjectionMatrix()
{
	return &m_projectionMatrix;
}

Mat4f CameraData::getViewProjectionMatrix(IGameEntity* parent)
{
	Mat4f l_cameraRotationMatrix = parent->caclTransformedRot().conjugate().toRotationMatrix();
	Vec3f l_cameraPosition = parent->caclTransformedPos() * -1.0f;
	Mat4f l_cameraTranslationMatrix;
	l_cameraTranslationMatrix.initTranslation(l_cameraPosition.getX(), l_cameraPosition.getY(),
		l_cameraPosition.getZ());
	return m_projectionMatrix * l_cameraRotationMatrix * l_cameraTranslationMatrix;
}

CameraComponent::CameraComponent()
{
}

CameraComponent::~CameraComponent()
{
}

Mat4f * CameraComponent::getProjectionMatrix()
{
	return m_cameraData.getProjectionMatrix();
}

Mat4f CameraComponent::getViewProjectionMatrix()
{
	return m_cameraData.getViewProjectionMatrix(this);
}

void CameraComponent::init()
{
	m_cameraData.addCameraData((90.0f / 180.0f) * PI, (16.0f / 9.0f), 0.1f, 1000.0f);
}

void CameraComponent::update()
{
}

void CameraComponent::shutdown()
{
}

