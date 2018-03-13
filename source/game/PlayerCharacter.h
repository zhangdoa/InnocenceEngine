#pragma once
#include "component/CameraComponent.h"
#include "component/InputComponent.h"
#include "component/VisibleComponent.h"

class PlayerCharacter : public BaseActor
{
public:
	PlayerCharacter();
	~PlayerCharacter();

	CameraComponent& getCameraComponent();
	InputComponent& getInputComponent();
	VisibleComponent& getVisibleComponent();
private:	
	InputComponent m_inputComponent;
	CameraComponent m_cameraComponent;
	VisibleComponent m_visibleComponent;
};

