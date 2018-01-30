#include "../../../main/stdafx.h"
#include "RenderingManager.h"

void RenderingManager::changeDrawPolygonMode()
{
	GLRenderingManager::getInstance().changeDrawPolygonMode();
}

void RenderingManager::changeDrawTextureMode()
{
	GLRenderingManager::getInstance().changeDrawTextureMode();
}

meshID RenderingManager::addMesh()
{
	return GLRenderingManager::getInstance().addMesh();
}

textureID RenderingManager::addTexture()
{
	return GLRenderingManager::getInstance().addTexture();
}

IMesh* RenderingManager::getMesh(meshID meshID)
{
	return GLRenderingManager::getInstance().getMesh(meshID);
}

I2DTexture* RenderingManager::getTexture(textureID textureID)
{
	return  GLRenderingManager::getInstance().getTexture(textureID);
}

void RenderingManager::setup()
{
	m_childManager.emplace_back(&GLWindowManager::getInstance());
	m_childManager.emplace_back(&GLInputManager::getInstance());
	m_childManager.emplace_back(&GLRenderingManager::getInstance());
	for (size_t i = 0; i < m_childManager.size(); i++)
	{
		m_childManager[i].get()->setup();
	}
	GLRenderingManager::getInstance().setScreenResolution(GLWindowManager::getInstance().getScreenResolution());

	f_changeDrawPolygonMode = std::bind(&RenderingManager::changeDrawPolygonMode, this);
	f_changeDrawTextureMode = std::bind(&RenderingManager::changeDrawTextureMode, this);
}

void RenderingManager::initialize()
{
	for (size_t i = 0; i < m_childManager.size(); i++)
	{
		m_childManager[i].get()->initialize();
	}

	for (size_t i = 0; i < SceneGraphManager::getInstance().getInputQueue().size(); i++)
	{
		// @TODO: multi input components need to register to multi map
		GLInputManager::getInstance().addKeyboardInputCallback(SceneGraphManager::getInstance().getInputQueue()[i]->getKeyboardInputCallbackImpl());
		GLInputManager::getInstance().addMouseMovementCallback(SceneGraphManager::getInstance().getInputQueue()[i]->getMouseInputCallbackImpl());
	}

	GLInputManager::getInstance().addKeyboardInputCallback(GLFW_KEY_Q, &f_changeDrawPolygonMode);
	GLInputManager::getInstance().addKeyboardInputCallback(GLFW_KEY_E, &f_changeDrawTextureMode);

	this->setStatus(objectStatus::ALIVE);
	LogManager::getInstance().printLog("RenderingManager has been initialized.");
}

void RenderingManager::update()
{
	AsyncRender();
	
if (GLWindowManager::getInstance().getStatus() == objectStatus::STANDBY)
	{
		this->setStatus(objectStatus::STANDBY);
		LogManager::getInstance().printLog("RenderingManager is stand-by.");
	}
}

void RenderingManager::shutdown()
{
	for (size_t i = 0; i < m_childManager.size(); i++)
	{
		// reverse 'destructor'
		m_childManager[m_childManager.size() - 1 - i].get()->shutdown();
	}
	for (size_t i = 0; i < m_childManager.size(); i++)
	{
		// reverse 'destructor'
		m_childManager[m_childManager.size() - 1 - i].release();
	}
	this->setStatus(objectStatus::SHUTDOWN);
	LogManager::getInstance().printLog("RenderingManager has been shutdown.");
}

void RenderingManager::AsyncRender()
{
	//prepare rendering global state
	GLRenderingManager::getInstance().update();

	//forward render
	//GLRenderingManager::getInstance().forwardRender(SceneGraphManager::getInstance().getCameraQueue(), SceneGraphManager::getInstance().getLightQueue(), SceneGraphManager::getInstance().getRenderingQueue());
	
	//defer render
	GLRenderingManager::getInstance().deferRender(SceneGraphManager::getInstance().getCameraQueue(), SceneGraphManager::getInstance().getLightQueue(), SceneGraphManager::getInstance().getRenderingQueue());
	
	GLWindowManager::getInstance().update();

	GLInputManager::getInstance().update();
}
