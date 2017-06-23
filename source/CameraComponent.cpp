#include "CameraComponent.h"



CameraComponent::CameraComponent()
{
	_projection.initPerspective(90, 16.0f / 9.0f, 0.1f, 1000.0f);
	_camera.setProjection(&_projection);
	getParent()->getRenderingEngine()->setMainCamera(&_camera);
}

CameraComponent::CameraComponent(float fov, float aspectRatio, float zNear, float zFar)
{
	_projection.initPerspective(fov, aspectRatio, zNear, zFar);
	_camera.setProjection(&_projection);
	getParent()->getRenderingEngine()->setMainCamera(&_camera);
}


CameraComponent::~CameraComponent()
{
}

