#include "stdafx.h"
#include "GraphicManager.h"

GraphicManager::GraphicManager()
{
}


GraphicManager::~GraphicManager()
{
}

void GraphicManager::render(IVisibleGameEntity * visibleGameEntity)
{
	m_renderingManager.render(visibleGameEntity);
}

void GraphicManager::setCameraViewProjectionMatrix(const glm::mat4& cameraViewProjectionMatrix)
{
	m_renderingManager.setCameraViewProjectionMatrix(cameraViewProjectionMatrix);
}

void GraphicManager::init()
{
	m_renderingManager.exec(INIT);
	this->setStatus(INITIALIZIED);
	LogManager::printLog("GraphicManager has been initialized.");
}

void GraphicManager::update()
{
	m_renderingManager.exec(UPDATE);
}

void GraphicManager::shutdown()
{
	m_renderingManager.exec(SHUTDOWN);
	this->setStatus(UNINITIALIZIED);
	LogManager::printLog("GraphicManager has been shutdown.");
}