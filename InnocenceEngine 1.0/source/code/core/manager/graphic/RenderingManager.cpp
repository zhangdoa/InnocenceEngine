#include "../../../main/stdafx.h"
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

void RenderingManager::getCameraTranslationMatrix(glm::mat4 & t) const
{
	GLRenderingManager::getInstance().getCameraTranslationMatrix(t);
}

void RenderingManager::setCameraTranslationMatrix(const glm::mat4 & t)
{
	GLRenderingManager::getInstance().setCameraTranslationMatrix(t);
}

void RenderingManager::getCameraViewMatrix(glm::mat4 & v) const
{
	GLRenderingManager::getInstance().getCameraViewMatrix(v);
}

void RenderingManager::setCameraViewMatrix(const glm::mat4 & v)
{
	GLRenderingManager::getInstance().setCameraViewMatrix(v);
}

void RenderingManager::getCameraProjectionMatrix(glm::mat4 & p) const
{
	GLRenderingManager::getInstance().getCameraProjectionMatrix(p);
}

void RenderingManager::setCameraProjectionMatrix(const glm::mat4 & p)
{
	GLRenderingManager::getInstance().setCameraProjectionMatrix(p);
}

void RenderingManager::init()
{
	GLRenderingManager::getInstance().exec(execMessage::INIT);
	this->setStatus(objectStatus::INITIALIZIED);
	LogManager::getInstance().printLog("RenderingManager has been initialized.");
}

void RenderingManager::update()
{
	GLRenderingManager::getInstance().exec(execMessage::UPDATE);
}

void RenderingManager::shutdown()
{
	GLRenderingManager::getInstance().exec(execMessage::SHUTDOWN);
	this->setStatus(objectStatus::UNINITIALIZIED);
	LogManager::getInstance().printLog("RenderingManager has been shutdown.");
}