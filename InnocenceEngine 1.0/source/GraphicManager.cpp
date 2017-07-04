#include "stdafx.h"
#include "GraphicManager.h"


GraphicManager::GraphicManager()
{
}


GraphicManager::~GraphicManager()
{
}


void GraphicManager::init()
{
	m_uiManager.exec(INIT);
	m_renderingManager.exec(INIT);
	this->setStatus(INITIALIZIED);
	printLog("GraphicManager has been initialized.");
}

void GraphicManager::update()
{
	if (m_uiManager.getStatus() == INITIALIZIED)
	{
		m_uiManager.exec(UPDATE);
		m_renderingManager.exec(UPDATE);
	}
	else
	{
		this->setStatus(STANDBY);
		printLog("GraphicManager is stand-by.");
	}
}

void GraphicManager::shutdown()
{
	m_renderingManager.exec(SHUTDOWN);
	m_uiManager.exec(SHUTDOWN);
	this->setStatus(UNINITIALIZIED);
	printLog("GraphicManager has been shutdown.");
}