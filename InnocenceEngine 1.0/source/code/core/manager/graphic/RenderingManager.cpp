#include "../../../main/stdafx.h"
#include "RenderingManager.h"

RenderingManager::RenderingManager()
{
}


RenderingManager::~RenderingManager()
{
}

GLWindowManager& RenderingManager::getWindowManager() const
{
	return GLWindowManager::getInstance();
}

GLInputManager& RenderingManager::getInputManager() const
{
	return GLInputManager::getInstance();
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

void RenderingManager::initialize()
{
	m_childEventManager.emplace_back(&GLWindowManager::getInstance());
	m_childEventManager.emplace_back(&GLInputManager::getInstance());
	m_childEventManager.emplace_back(&GLRenderingManager::getInstance());
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[i].get()->excute(executeMessage::INITIALIZE);
	}
	this->setStatus(objectStatus::ALIVE);
	LogManager::getInstance().printLog("RenderingManager has been initialized.");
}

void RenderingManager::update()
{
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[i].get()->excute(executeMessage::UPDATE);
	}
	if (GLWindowManager::getInstance().getStatus() == objectStatus::STANDBY)
	{
		this->setStatus(objectStatus::STANDBY);
		LogManager::getInstance().printLog("RenderingManager is stand-by.");
	}
}

void RenderingManager::shutdown()
{
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		// reverse 'destructor'
		m_childEventManager[m_childEventManager.size() - 1 - i].get()->excute(executeMessage::SHUTDOWN);
	}
	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		// reverse 'destructor'
		m_childEventManager[m_childEventManager.size() - 1 - i].release();
	}
	this->setStatus(objectStatus::SHUTDOWN);
	LogManager::getInstance().printLog("RenderingManager has been shutdown.");
}