#include "../main/stdafx.h"
#include "PlayerCharacter.h"


PlayerCharacter::PlayerCharacter()
{
	addChildComponent(&m_inputComponent);
	addChildComponent(&m_cameraComponent);
}


PlayerCharacter::~PlayerCharacter()
{
}

CameraComponent & PlayerCharacter::getCameraComponent()
{
	return m_cameraComponent;
}

InputComponent & PlayerCharacter::getInputComponent()
{
	return m_inputComponent;
}
