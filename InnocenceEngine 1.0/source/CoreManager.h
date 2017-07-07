#pragma once

#include "IEventManager.h"
#include "LogManager.h"
#include "TimeManager.h"
#include "WindowManager.h"
#include "InputManager.h"
#include "GraphicManager.h"
#include "SceneGraphManager.h"
#include "IGameData.h"

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
	
	std::vector<std::auto_ptr<IEventManager>> m_childEventManager;

	//TimeManager m_timeManager;
	//WindowManager m_windowManager;
	//InputManager m_inputManager;
	//GraphicManager m_graphicManager;
	//SceneGraphManager m_sceneGraphManager;
	IGameData* m_gameData;
};

