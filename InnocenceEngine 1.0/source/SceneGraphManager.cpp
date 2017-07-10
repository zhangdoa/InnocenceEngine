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
	LogManager::getInstance().printLog("SceneGraphManager has been initialized.");
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
		LogManager::getInstance().printLog("SceneGraphManager is stand-by.");
	}
}

void SceneGraphManager::shutdown()
{
	m_rootActor.exec(SHUTDOWN);
	LogManager::getInstance().printLog("SceneGraphManager has been shutdown.");
}
