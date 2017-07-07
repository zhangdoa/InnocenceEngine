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
	virtual void setInputManager(InputManager* inputManager);
	virtual CameraComponent* getCameraComponent() = 0;
	virtual IVisibleGameEntity* getTest() = 0;

protected:
	InputManager* getInputManager();
private:
	InputManager* m_inputManager;
};

#endif // !_I_GAME_DATA_H_
