#include "../../main/stdafx.h"
#include "Win32Application.h"

void Win32Application::setup()
{
	m_ipGameData = &m_gameData;
	CoreManager::getInstance().setGameData(m_ipGameData);
	CoreManager::getInstance().setup();
}

void Win32Application::initialize()
{
	CoreManager::getInstance().initialize();
	setStatus(objectStatus::ALIVE);
}

void Win32Application::update()
{
	if (CoreManager::getInstance().getStatus() == objectStatus::ALIVE)
	{
		CoreManager::getInstance().update();
	}
	else
	{
		setStatus(objectStatus::STANDBY);
	}
}

void Win32Application::shutdown()
{
	CoreManager::getInstance().shutdown();
	setStatus(objectStatus::SHUTDOWN);
}
