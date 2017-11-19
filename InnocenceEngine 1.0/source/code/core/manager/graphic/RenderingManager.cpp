#include "../../../main/stdafx.h"
#include "RenderingManager.h"

RenderingManager::RenderingManager()
{
}


RenderingManager::~RenderingManager()
{
}

void RenderingManager::initInput()
{
	for (size_t i = 0; i < SceneGraphManager::getInstance().getInputQueue().size(); i++)
	{
		// @TODO: multi input components need to register to multi map
		GLInputManager::getInstance().setKeyboardInputCallback(SceneGraphManager::getInstance().getInputQueue()[i]->getKeyboardInputCallbackImpl());
		GLInputManager::getInstance().setMouseMovementCallback(SceneGraphManager::getInstance().getInputQueue()[i]->getMouseInputCallbackImpl());
	}
}

void RenderingManager::changeDrawPolygonMode() const
{
	GLRenderingManager::getInstance().changeDrawPolygonMode();
}

void RenderingManager::toggleDepthBufferVisualizer()
{
	GLRenderingManager::getInstance().toggleDepthBufferVisualizer();
}

void RenderingManager::initialize()
{

	m_childEventManager.emplace_back(&GLWindowManager::getInstance());
	m_childEventManager.emplace_back(&GLInputManager::getInstance());
	m_childEventManager.emplace_back(&GLRenderingManager::getInstance());
	//m_childEventManager.emplace_back(&GLGUIManager::getInstance());

	GLRenderingManager::getInstance().setScreenResolution(GLWindowManager::getInstance().getScreenResolution());

	for (size_t i = 0; i < m_childEventManager.size(); i++)
	{
		m_childEventManager[i].get()->excute(executeMessage::INITIALIZE);
	}

	//m_asyncRenderThread = new std::thread(&RenderingManager::AsyncRender, this);
	
	this->setStatus(objectStatus::ALIVE);
	LogManager::getInstance().printLog("RenderingManager has been initialized.");
}

void RenderingManager::update()
{
	//if (m_asyncRenderThread->joinable())
	//{
	//	m_asyncRenderThread->join();
	//}
	AsyncRender();
	
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

void RenderingManager::AsyncRender()
{
	//prepare rendering global state
	GLRenderingManager::getInstance().excute(executeMessage::UPDATE);

	////forward render
	//GLRenderingManager::getInstance().forwardRender(SceneGraphManager::getInstance().getCameraQueue(), SceneGraphManager::getInstance().getLightQueue(), SceneGraphManager::getInstance().getRenderingQueue());
	
	//defer render
	GLRenderingManager::getInstance().deferRender(SceneGraphManager::getInstance().getCameraQueue(), SceneGraphManager::getInstance().getLightQueue(), SceneGraphManager::getInstance().getRenderingQueue());
	
	GLWindowManager::getInstance().excute(executeMessage::UPDATE);

	GLInputManager::getInstance().excute(executeMessage::UPDATE);
	//GLGUIManager::getInstance().excute(executeMessage::UPDATE);

	//LogManager::getInstance().printLog("Async Rendering Finished.");
}
