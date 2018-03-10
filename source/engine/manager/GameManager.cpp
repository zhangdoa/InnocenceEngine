#include "GameManager.h"

void GameManager::setup()
{
	if (g_pGame)
	{
		g_pLogManager->printLog("Game loaded.");
		g_pGame->setup();

		m_CameraComponents = g_pGame->getCameraComponents();
		m_InputComponents = g_pGame->getInputComponents();
		m_LightComponents = g_pGame->getLightComponents();
		m_VisibleComponents = g_pGame->getVisibleComponents();

		g_pLogManager->printLog("Game setup finished.");
		this->setStatus(objectStatus::ALIVE);
	}
	else
	{
		g_pLogManager->printLog("No game loaded!");
		this->setStatus(objectStatus::STANDBY);
	}
}

void GameManager::initialize()
{
	g_pGame->initialize();
}

void GameManager::update()
{
	g_pGame->update();
}

void GameManager::shutdown()
{
}

std::vector<VisibleComponent*>& GameManager::getVisibleComponents()
{
	return m_VisibleComponents;
}

std::vector<LightComponent*>& GameManager::getLightComponents()
{
	return m_LightComponents;
}

std::vector<CameraComponent*>& GameManager::getCameraComponents()
{
	return m_CameraComponents;
}

std::vector<InputComponent*>& GameManager::getInputComponents()
{
	return m_InputComponents;
}

std::string GameManager::getGameName() const
{
	return g_pGame->getGameName();
}

bool GameManager::needRender()
{
	return m_needRender;
}

const objectStatus & GameManager::getStatus() const
{
	return m_objectStatus;
}

void GameManager::setStatus(objectStatus objectStatus)
{
	m_objectStatus = objectStatus;
}
