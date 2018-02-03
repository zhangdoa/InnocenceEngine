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

textureID RenderingManager::add2DTexture()
{
	return GLRenderingManager::getInstance().add2DTexture();
}

textureID RenderingManager::add2DHDRTexture()
{
	return  GLRenderingManager::getInstance().add2DHDRTexture();
}

textureID RenderingManager::add3DTexture()
{
	return GLRenderingManager::getInstance().add3DTexture();
}

textureID RenderingManager::add3DHDRTexture()
{
	return  GLRenderingManager::getInstance().add3DHDRTexture();
}

IMesh* RenderingManager::getMesh(meshID meshID)
{
	return GLRenderingManager::getInstance().getMesh(meshID);
}

I2DTexture* RenderingManager::get2DTexture(textureID textureID)
{
	return  GLRenderingManager::getInstance().get2DTexture(textureID);
}

I2DTexture * RenderingManager::get2DHDRTexture(textureID textureID)
{
	return GLRenderingManager::getInstance().get2DHDRTexture(textureID);
}

I3DTexture * RenderingManager::get3DTexture(textureID textureID)
{
	return GLRenderingManager::getInstance().get3DTexture(textureID);
}

I3DTexture * RenderingManager::get3DHDRTexture(textureID textureID)
{
	return GLRenderingManager::getInstance().get3DHDRTexture(textureID);
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
