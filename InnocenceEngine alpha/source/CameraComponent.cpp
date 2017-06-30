#include "CameraComponent.h"



CameraComponent::CameraComponent()
{
	this->_projection = Mat4f();
	_projection.initPerspective(90, 16.0f / 9.0f, 0.1f, 1000.0f);
}

CameraComponent::CameraComponent(float fov, float aspectRatio, float zNear, float zFar)
{
	this->_projection = Mat4f();
	_projection.initPerspective(fov, aspectRatio, zNear, zFar);
}


CameraComponent::~CameraComponent()
{
}

Mat4f CameraComponent::getViewProjection()
{
	Mat4f cameraRotation = getParent()->caclTransformedRot().conjugate().toRotationMatrix();
	Vec3f cameraPos = getParent()->caclTransformedPos() * -1.0f;
	Mat4f cameraTranslation;
	cameraTranslation.initTranslation(cameraPos.getX(), cameraPos.getY(),
		cameraPos.getZ());
	return _projection * cameraRotation * cameraTranslation;
}

