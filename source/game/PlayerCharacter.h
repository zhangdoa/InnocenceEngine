#pragma once
#include "component/CameraComponent.h"
#include "component/InputComponent.h"

class PlayerCharacter : public BaseActor
{
public:
	PlayerCharacter();
	~PlayerCharacter();

	CameraComponent& getCameraComponent();
	InputComponent& getInputComponent();
private:	
	InputComponent m_inputComponent;
	CameraComponent m_cameraComponent;
};

