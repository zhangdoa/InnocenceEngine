#include "stdafx.h"
#include "GraphicManager.h"

GraphicManager::GraphicManager()
{
}


GraphicManager::~GraphicManager()
{
}

void GraphicManager::render(IVisibleGameEntity * visibleGameEntity) const
{
	GLRenderingManager::getInstance().render(visibleGameEntity);
}

void GraphicManager::setCamera(CameraComponent* cameraComponent)
{
	GLRenderingManager::getInstance().setCamera(cameraComponent);
}

void GraphicManager::init()
{
	GLRenderingManager::getInstance().exec(INIT);
	this->setStatus(INITIALIZIED);
	LogManager::getInstance().printLog("GraphicManager has been initialized.");
}

void GraphicManager::update()
{
	GLRenderingManager::getInstance().exec(UPDATE);
}

void GraphicManager::shutdown()
{
	GLRenderingManager::getInstance().exec(SHUTDOWN);
	this->setStatus(UNINITIALIZIED);
	LogManager::getInstance().printLog("GraphicManager has been shutdown.");
}