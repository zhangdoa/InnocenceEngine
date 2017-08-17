#include "../../main/stdafx.h"
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

TimeManager & CoreManager::getTimeManager() const
{
	return TimeManager::getInstance();
}

WindowManager & CoreManager::getWindowManager() const
{
	return WindowManager::getInstance();
}

InputManager & CoreManager::getInputManager() const
{
	return InputManager::getInstance();
}

RenderingManager & CoreManager::getRenderingManager() const
{
	return RenderingManager::getInstance();
}

LogManager & CoreManager::getLogManager() const
{
	return LogManager::getInstance();
}

void CoreManager::init()
{
	// emplace_back in a static order.
	m_childEventManager.emplace_back(&AssetManager::getInstance());
	m_childEventManager.emplace_back(&TimeManager::getInstance());
	m_childEventManager.emplace_back(&WindowManager::getInstance());
	m_childEventManager.emplace_back(&InputManager::getInstance());
	m_childEventManager.emplace_back(&RenderingManager::getInstance());
	m_childEventManager.emplace_back(&GUIManager::getInstance());
	//m_childEventManager.emplace_back(&SceneGraphManager::getInstance());

	std::string l_gameName;
	m_gameData->getGameName(l_gameName);

	WindowManager::getInstance().setWindowName(l_gameName);

	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[i].get()->exec(execMessage::INIT);
	}

	try {
		m_gameData->exec(execMessage::INIT);
	}
	catch (std::exception& e) {
		LogManager::getInstance().printLog("No game added!");
		LogManager::getInstance().printLog(e.what());
	}

	this->setStatus(objectStatus::INITIALIZIED);
	LogManager::getInstance().printLog("CoreManager has been initialized.");
}

void CoreManager::update()
{
	// time manager should update without any limitation.
	TimeManager::getInstance().exec(execMessage::UPDATE);

	// when time manager's status was execMessage::INITIALIZED, that means we can update other managers, a frame counter occurred.
	if (TimeManager::getInstance().getStatus() == objectStatus::INITIALIZIED)
	{

		if (WindowManager::getInstance().getStatus() == objectStatus::INITIALIZIED)
		{
			// input manager decides the game& engine behivour which was based on user's input.
			InputManager::getInstance().exec(execMessage::UPDATE);

			if (InputManager::getInstance().getStatus() == objectStatus::INITIALIZIED)
			{
				RenderingManager::getInstance().exec(execMessage::UPDATE);

				// game data update
				m_gameData->exec(execMessage::UPDATE);
				// update global rendering status
				
				//TODO: invoke finishRender() at the end of multi-thread rendering pipeline
				RenderingManager::getInstance().finishRender();

				//SceneGraphManager::getInstance().exec(execMessage::UPDATE);
				//GUIManager::getInstance().exec(execMessage::UPDATE);
				// window manager updates last
				WindowManager::getInstance().exec(execMessage::UPDATE);
			}
			else
			{
				this->setStatus(objectStatus::STANDBY);
				LogManager::getInstance().printLog("CoreManager is stand-by.");
			}
		}
		else
		{
			this->setStatus(objectStatus::STANDBY);
			LogManager::getInstance().printLog("CoreManager is stand-by.");
		}
	}
}

void CoreManager::shutdown()
{
	m_gameData->exec(execMessage::SHUTDOWN);
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		// reverse 'destructor'
		m_childEventManager[m_childEventManager.size() - 1 - i].get()->exec(execMessage::SHUTDOWN);
	}
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		// reverse 'destructor'
		m_childEventManager[m_childEventManager.size() - 1 - i].release();
	}
	this->setStatus(objectStatus::INITIALIZIED);
	LogManager::getInstance().printLog("CoreManager has been shutdown.");
	std::this_thread::sleep_for(std::chrono::seconds(5));
}
