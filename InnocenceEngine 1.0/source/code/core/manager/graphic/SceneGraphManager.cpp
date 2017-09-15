#include "../../../main/stdafx.h"
#include "SceneGraphManager.h"


SceneGraphManager::SceneGraphManager()
{
}


SceneGraphManager::~SceneGraphManager()
{
}

void SceneGraphManager::initialize()
{
	this->setStatus(objectStatus::ALIVE);
	LogManager::getInstance().printLog("SceneGraphManager has been initialized.");
}

void SceneGraphManager::update()
{
	if (m_rootActor.getStatus() == objectStatus::ALIVE)
	{
		m_rootActor.excute(executeMessage::UPDATE);
	}
	else
	{
		this->setStatus(objectStatus::STANDBY);
		LogManager::getInstance().printLog("SceneGraphManager is stand-by.");
	}
}

void SceneGraphManager::shutdown()
{
	m_rootActor.excute(executeMessage::SHUTDOWN);
	LogManager::getInstance().printLog("SceneGraphManager has been shutdown.");
}
