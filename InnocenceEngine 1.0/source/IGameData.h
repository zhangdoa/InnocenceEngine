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
	const std::string getGameName();
};

#endif // !_I_GAME_DATA_H_
