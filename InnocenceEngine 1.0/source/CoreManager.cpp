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
	m_childEventManager.emplace_back(&m_timeManager);
	m_childEventManager.emplace_back(&m_windowManager);
	m_childEventManager.emplace_back(&m_inputManager);
	m_childEventManager.emplace_back(&m_graphicManager);
	m_childEventManager.emplace_back(&m_sceneGraphManager);

	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[i].get()->exec(INIT);
	}

	try {
		m_inputManager.setWindow(m_windowManager.getWindow());
	}
	catch (std::exception& e) {
		LogManager::printLog("Set window instance for InputManager failed!");
		LogManager::printLog(e.what());
	}

	try {
		m_gameData->exec(INIT);
	}
	catch (std::exception& e) {
		LogManager::printLog("No game added!");
		LogManager::printLog(e.what());
	}

	try {
		m_graphicManager.setCameraProjectionMatrix(m_gameData->getCameraComponent()->getProjectionMatrix());
		m_graphicManager.setCameraViewProjectionMatrix(m_gameData->getCameraComponent()->getViewProjectionMatrix());
	}
	catch (std::exception& e) {
		LogManager::printLog("Cannot get camera infomation!");
		LogManager::printLog(e.what());
	}

	this->setStatus(INITIALIZIED);
	LogManager::printLog("CoreManager has been initialized.");
}

void CoreManager::update()
{
	// time manager should update without any limit.
	m_timeManager.exec(UPDATE);

	// when time manager's status was INITIALIZED, that means we can update the other manager, a frame counter occured.
	if (m_timeManager.getStatus() == INITIALIZIED)
	{
		// window manager updates first, due to I use GLFW lib to manage the windows event currently.
		m_windowManager.exec(UPDATE);

		if (m_windowManager.getStatus() == INITIALIZIED)
		{
			// input manager decides the game& engine behivour next steps which was based on user's input.
			m_inputManager.exec(UPDATE);

			if (m_inputManager.getStatus() == INITIALIZIED)
			{
				m_graphicManager.exec(UPDATE);
				m_gameData->exec(UPDATE);
				m_sceneGraphManager.exec(UPDATE);
			}
			else
			{
				this->setStatus(STANDBY);
				LogManager::printLog("CoreManager is stand-by.");
			}
		}
		else
		{
			this->setStatus(STANDBY);
			LogManager::printLog("CoreManager is stand-by.");
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
	this->setStatus(UNINITIALIZIED);
	LogManager::printLog("CoreManager has been shutdown.");
	std::this_thread::sleep_for(std::chrono::seconds(5));
}
