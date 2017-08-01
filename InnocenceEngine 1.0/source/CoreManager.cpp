#include "stdafx.h"
#include "CoreManager.h"


CoreManager::CoreManager()
{
}


CoreManager::~CoreManager()
{
}

void CoreManager::setGameData(IGameData * gameData)
{
	m_gameData = gameData;
}

void CoreManager::init()
{
	// emplace_back in a static order.
	m_childEventManager.emplace_back(&TimeManager::getInstance());
	m_childEventManager.emplace_back(&WindowManager::getInstance());
	m_childEventManager.emplace_back(&InputManager::getInstance());
	m_childEventManager.emplace_back(&RenderingManager::getInstance());
	//m_childEventManager.emplace_back(&SceneGraphManager::getInstance());

	std::string l_gameName;
	m_gameData->getGameName(l_gameName);

	WindowManager::getInstance().setWindowName(l_gameName);

	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[i].get()->exec(INIT);
	}

	try {
		m_gameData->exec(INIT);
	}
	catch (std::exception& e) {
		LogManager::getInstance().printLog("No game added!");
		LogManager::getInstance().printLog(e.what());
	}

	this->setStatus(INITIALIZIED);
	LogManager::getInstance().printLog("CoreManager has been initialized.");
}

void CoreManager::update()
{
	// time manager should update without any limitation.
	TimeManager::getInstance().exec(UPDATE);

	// when time manager's status was INITIALIZED, that means we can update other managers, a frame counter occurred.
	if (TimeManager::getInstance().getStatus() == INITIALIZIED)
	{

		if (WindowManager::getInstance().getStatus() == INITIALIZIED)
		{
			// input manager decides the game& engine behivour which was based on user's input.
			InputManager::getInstance().exec(UPDATE);

			if (InputManager::getInstance().getStatus() == INITIALIZIED)
			{
				// game data update
				m_gameData->exec(UPDATE);
				
				// update global rendering status
				RenderingManager::getInstance().exec(UPDATE);

				try {
					// update camera info for rendering pipeline
					InnocenceGarden* m_gameDataSpecific = dynamic_cast<InnocenceGarden*>(m_gameData);
					RenderingManager::getInstance().setCamera(m_gameDataSpecific->getCameraComponent());

					// rendering pipeline starts to work
					RenderingManager::getInstance().render(m_gameDataSpecific->getTestCube());
					RenderingManager::getInstance().render(m_gameDataSpecific->getSkybox());

					RenderingManager::getInstance().finishRender();
				}
				catch (std::exception& e) {
					LogManager::getInstance().printLog("Cannot get camera information!");
					LogManager::getInstance().printLog(e.what());
				}



				//SceneGraphManager::getInstance().exec(UPDATE);

				// window manager updates last
				WindowManager::getInstance().exec(UPDATE);
			}
			else
			{
				this->setStatus(STANDBY);
				LogManager::getInstance().printLog("CoreManager is stand-by.");
			}
		}
		else
		{
			this->setStatus(STANDBY);
			LogManager::getInstance().printLog("CoreManager is stand-by.");
		}
	}
}

void CoreManager::shutdown()
{
	m_gameData->exec(SHUTDOWN);
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		// reverse 'destructor'
		m_childEventManager[m_childEventManager.size() - 1 - i].get()->exec(SHUTDOWN);
	}
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		// reverse 'destructor'
		m_childEventManager[m_childEventManager.size() - 1 - i].release();
	}
	this->setStatus(UNINITIALIZIED);
	LogManager::getInstance().printLog("CoreManager has been shutdown.");
	std::this_thread::sleep_for(std::chrono::seconds(5));
}
