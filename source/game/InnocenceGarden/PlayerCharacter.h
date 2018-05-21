#pragma once
#include "component/TransformComponent.h"
#include "component/CameraComponent.h"
#include "component/InputComponent.h"
#include "component/VisibleComponent.h"
#include "entity/BaseEntity.h"

class PlayerCharacter : public BaseEntity
{
public:
	PlayerCharacter();
	~PlayerCharacter();

	TransformComponent& getTransformComponent();
	CameraComponent& getCameraComponent();
	InputComponent& getInputComponent();
	VisibleComponent& getVisibleComponent();

private:	
	TransformComponent m_transformCompoent;
	InputComponent m_inputComponent;
	CameraComponent m_cameraComponent;
	VisibleComponent m_visibleComponent;
};

