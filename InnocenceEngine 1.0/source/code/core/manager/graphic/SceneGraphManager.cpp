#include "../../../main/stdafx.h"
#include "SceneGraphManager.h"


SceneGraphManager::SceneGraphManager()
{
}


SceneGraphManager::~SceneGraphManager()
{
}

void SceneGraphManager::init()
{
	m_rootActor.exec(execMessage::INIT);
	this->setStatus(objectStatus::INITIALIZIED);
	LogManager::getInstance().printLog("SceneGraphManager has been initialized.");
}

void SceneGraphManager::update()
{
	if (m_rootActor.getStatus() == objectStatus::INITIALIZIED)
	{
		m_rootActor.exec(execMessage::UPDATE);
	}
	else
	{
		this->setStatus(objectStatus::STANDBY);
		LogManager::getInstance().printLog("SceneGraphManager is stand-by.");
	}
}

void SceneGraphManager::shutdown()
{
	m_rootActor.exec(execMessage::SHUTDOWN);
	LogManager::getInstance().printLog("SceneGraphManager has been shutdown.");
}
