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

Mat4f CameraData::getViewProjectionMatrix(IGameEntity* parent)
{
	Mat4f l_cameraRotationMatrix = parent->caclTransformedRot().getConjugatedVec4f().toRotationMatrix();

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

Mat4f CameraComponent::getViewProjectionMatrix()
{
	return m_cameraData.getViewProjectionMatrix(this);
}

void CameraComponent::move(moveDirection moveDirection)
{
	switch (moveDirection)
	{
	case FORWARD:  this->getTransform()->setPos(this->getTransform()->getPos() + this->getTransform()->getRot().getForward() * 0.03f);  break;
	case BACKWARD:  this->getTransform()->setPos(this->getTransform()->getPos() + this->getTransform()->getRot().getForward() * -0.03f);  break;
	case LEFT:   this->getTransform()->setPos(this->getTransform()->getPos() + this->getTransform()->getRot().getRight() * -0.03f);  break;
	case RIGHT:   this->getTransform()->setPos(this->getTransform()->getPos() + this->getTransform()->getRot().getRight() * 0.03f);  break;
	}

}

Vec3f * CameraComponent::getYAxis()
{
	return &m_cameraData.yAxis;
}

void CameraComponent::init()
{
	m_cameraData.addCameraData((90.0f / 180.0f) * PI, (16.0f / 9.0f), 0.1f, 1000.0f);
}

void CameraComponent::update()
{
	getTransform()->update();
}

void CameraComponent::shutdown()
{
}

