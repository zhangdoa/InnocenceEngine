#pragma once

#include "IBaseObject.h"
#include "../manager/InputManager.h"
#include "../component/CameraComponent.h"
#include "IVisibleGameEntity.h"

#ifndef _I_GAME_DATA_H_
#define _I_GAME_DATA_H_

class IGameData : public IBaseObject
{
public:
	IGameData();
	virtual ~IGameData();
	void getGameName(std::string& gameName) const;
};

#endif // !_I_GAME_DATA_H_
