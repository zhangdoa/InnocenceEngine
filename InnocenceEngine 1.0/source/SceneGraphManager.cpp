#include "stdafx.h"
#include "SceneGraphManager.h"


SceneGraphManager::SceneGraphManager()
{
}


SceneGraphManager::~SceneGraphManager()
{
}

void SceneGraphManager::init()
{
	m_rootActor.exec(INIT);
	this->setStatus(INITIALIZIED);
	LogManager::printLog("SceneGraphManager has been initialized.");
}

void SceneGraphManager::update()
{
	if (m_rootActor.getStatus() == INITIALIZIED)
	{
		m_rootActor.exec(UPDATE);
	}
	else
	{
		this->setStatus(STANDBY);
		LogManager::printLog("SceneGraphManager is stand-by.");
	}
}

void SceneGraphManager::shutdown()
{
	m_rootActor.exec(SHUTDOWN);
	LogManager::printLog("SceneGraphManager has been shutdown.");
}
