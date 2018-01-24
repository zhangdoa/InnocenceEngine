#pragma once
#include "../core/component/CameraComponent.h"
#include "../core/component/InputComponent.h"

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

