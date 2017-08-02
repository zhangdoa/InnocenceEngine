#include "../../main/stdafx.h"
#include "RenderingManager.h"

RenderingManager::RenderingManager()
{
}


RenderingManager::~RenderingManager()
{
}

void RenderingManager::render(IVisibleGameEntity * visibleGameEntity) const
{
	GLRenderingManager::getInstance().render(visibleGameEntity);
}

void RenderingManager::finishRender() const
{
	GLRenderingManager::getInstance().finishRender();
}

void RenderingManager::setCamera(CameraComponent* cameraComponent)
{
	GLRenderingManager::getInstance().setCamera(cameraComponent);
}

void RenderingManager::init()
{
	GLRenderingManager::getInstance().exec(INIT);
	this->setStatus(INITIALIZIED);
	LogManager::getInstance().printLog("RenderingManager has been initialized.");
}

void RenderingManager::update()
{
	GLRenderingManager::getInstance().exec(UPDATE);
}

void RenderingManager::shutdown()
{
	GLRenderingManager::getInstance().exec(SHUTDOWN);
	this->setStatus(UNINITIALIZIED);
	LogManager::getInstance().printLog("RenderingManager has been shutdown.");
}