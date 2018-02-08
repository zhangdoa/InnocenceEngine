#pragma once
#include "../engine/component/CameraComponent.h"
#include "../engine/component/InputComponent.h"

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

