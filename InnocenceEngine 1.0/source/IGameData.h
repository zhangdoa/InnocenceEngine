#pragma once

#include "IBaseObject.h"
#include "InputManager.h"
#include "CameraComponent.h"
#include "IVisibleGameEntity.h"

#ifndef _I_GAME_DATA_H_
#define _I_GAME_DATA_H_

class IGameData : public IBaseObject
{
public:
	IGameData();
	virtual ~IGameData();
	virtual const std::string& getGameName() = 0;
	virtual CameraComponent* getCameraComponent() = 0;
	virtual IVisibleGameEntity* getTest() = 0;
	virtual IVisibleGameEntity* getSkybox() = 0;
};

#endif // !_I_GAME_DATA_H_
