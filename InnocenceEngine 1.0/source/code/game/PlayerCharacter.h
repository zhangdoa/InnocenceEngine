#pragma once
#include "../core/interface/IGameEntity.h"
#include "../core/component/CameraComponent.h"
#include "../core/component/InputComponent.h"

class PlayerCharacter : public BaseActor
{
public:
	PlayerCharacter();
	~PlayerCharacter();

private:	
	InputComponent inputComponent;
	CameraComponent cameraComponent;
	
	void init() override;
	void update() override;
	void shutdown() override;
};

