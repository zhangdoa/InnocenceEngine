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
	m_childEventManager.emplace_back(&GraphicManager::getInstance());
	//m_childEventManager.emplace_back(&SceneGraphManager::getInstance());

	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[i].get()->exec(INIT);
	}

	try {
		m_gameData->exec(INIT);
	}
	catch (std::exception& e) {
		LogManager::printLog("No game added!");
		LogManager::printLog(e.what());
	}

	this->setStatus(INITIALIZIED);
	LogManager::printLog("CoreManager has been initialized.");
}

void CoreManager::update()
{
	// time manager should update without any limitation.
	TimeManager::getInstance().exec(UPDATE);

	// when time manager's status was INITIALIZED, that means we can update other managers, a frame counter occurred.
	if (TimeManager::getInstance().getStatus() == INITIALIZIED)
	{
		// window manager updates first, because I use GLFW lib to manage the windows event currently.
		WindowManager::getInstance().exec(UPDATE);

		if (WindowManager::getInstance().getStatus() == INITIALIZIED)
		{
			// input manager decides the game& engine behivour next steps which was based on user's input.
			InputManager::getInstance().exec(UPDATE);

			if (InputManager::getInstance().getStatus() == INITIALIZIED)
			{
				try {
					//GraphicManager::getInstance().setCameraViewProjectionMatrix(m_gameData->getCameraComponent()->getViewProjectionMatrix());
				}
				catch (std::exception& e) {
					LogManager::printLog("Cannot get camera infomation!");
					LogManager::printLog(e.what());
				}
				GraphicManager::getInstance().exec(UPDATE);
				GraphicManager::getInstance().render(m_gameData->getTest());
				//m_gameData->getTest()->getTransform()->setPos(glm::vec3(m_gameData->getTest()->getTransform()->getPos().x + glm::sin(TimeManager::getInstance().getDeltaTime()), m_gameData->getTest()->getTransform()->getPos().y, m_gameData->getTest()->getTransform()->getPos().z));
				m_gameData->getTest()->getTransform()->rotate(glm::vec3(0.0f, 0.0f, 1.0f), 0.3 * glm::sin(TimeManager::getInstance().getDeltaTime()));
				m_gameData->getTest()->getTransform()->setScale(glm::vec3(0.5f, 0.5f, 1.0f));
				m_gameData->getTest()->exec(UPDATE);
				//SceneGraphManager::getInstance().exec(UPDATE);
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
