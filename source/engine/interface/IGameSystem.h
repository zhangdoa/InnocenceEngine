#pragma once
#include "common/stdafx.h"
#include "ISystem.h"
#include "ILogSystem.h"
#include "IGame.h"

extern ILogSystem* g_pLogSystem;
extern IGame* g_pGame;

class IGameSystem : public ISystem
{
public:
	virtual ~IGameSystem() {};

	virtual std::vector<VisibleComponent*>& getVisibleComponents() = 0;
	virtual std::vector<LightComponent*>& getLightComponents() = 0;
	virtual std::vector<CameraComponent*>& getCameraComponents() = 0;
	virtual std::vector<InputComponent*>& getInputComponents() = 0;

	virtual std::string getGameName() const = 0;
	virtual bool needRender() = 0;
};