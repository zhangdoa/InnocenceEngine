#include "CameraComponent.h"



CameraComponent::CameraComponent(float fov, float aspectRatio, float zNear, float zFar)
{
	projection = Mat4f().initPerspective(fov, aspectRatio, zNear, zFar);
}


CameraComponent::~CameraComponent()
{
}

Mat4f CameraComponent::getViewProjection()
{
	Mat4f cameraRotation = getTransform()->getTransformedRot().conjugate().toRotationMatrix();
	Vec3f cameraPos = getTransform()->getTransformedPos() * -1.0f;
	Mat4f cameraTranslation = Mat4f().initTranslation(cameraPos.getX(), cameraPos.getY(),
		cameraPos.getZ());
	return projection * cameraRotation * cameraTranslation;
}

void CameraComponent::addToRenderingEngine(RenderingEngine* renderingEngine) 
{
	renderingEngine->addCamera(this);
}
