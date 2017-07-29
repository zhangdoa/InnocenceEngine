#pragma once

#include "IEventManager.h"
#include "LogManager.h"
#include "TimeManager.h"
#include "WindowManager.h"
#include "InputManager.h"
#include "GraphicManager.h"
#include "SceneGraphManager.h"
#include "IGameData.h"
#include "InnocenceGarden.h"

class CoreManager : public IEventManager
{
public:
	CoreManager();
	~CoreManager();

	void setGameData(IGameData* gameData);

private:
	void init() override;
	void update() override;
	void shutdown() override;
	
	std::vector<std::unique_ptr<IEventManager>> m_childEventManager;

	IGameData* m_gameData;
};

