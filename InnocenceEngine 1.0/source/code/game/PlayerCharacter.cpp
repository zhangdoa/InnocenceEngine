#include "../main/stdafx.h"
#include "PlayerCharacter.h"


PlayerCharacter::PlayerCharacter()
{
	addChildComponent(&inputComponent);
	addChildComponent(&cameraComponent);
}


PlayerCharacter::~PlayerCharacter()
{
}
