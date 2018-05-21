#include "PlayerCharacter.h"


PlayerCharacter::PlayerCharacter()
{
	addChildComponent(&m_transformCompoent);
	addChildComponent(&m_cameraComponent);
	addChildComponent(&m_inputComponent);
	addChildComponent(&m_visibleComponent);

	m_cameraComponent.m_FOV = 60.0;
	m_cameraComponent.m_WHRatio = 16.0 / 9.0;
	m_cameraComponent.m_zNear = 0.1;
	m_cameraComponent.m_zFar = 200.0;
}


PlayerCharacter::~PlayerCharacter()
{
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
