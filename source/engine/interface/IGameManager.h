#pragma once
#include "common/stdafx.h"
#include "IManager.h"
#include "ILogManager.h"
#include "IGame.h"

extern ILogManager* g_pLogManager;
extern IGame* g_pGame;

class IGameManager : public IManager
{
public:
	virtual ~IGameManager() {};

	virtual std::vector<VisibleComponent*>& getVisibleComponents() = 0;
	virtual std::vector<LightComponent*>& getLightComponents() = 0;
	virtual std::vector<CameraComponent*>& getCameraComponents() = 0;
	virtual std::vector<InputComponent*>& getInputComponents() = 0;

	virtual std::string getGameName() const = 0;
	virtual bool needRender() = 0;
};