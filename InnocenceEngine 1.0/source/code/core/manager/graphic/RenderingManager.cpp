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

meshID RenderingManager::addMeshData()
{
	//@TODO: dangling pointer problem here
	GLMesh newMesh;
	m_meshMap.emplace(std::pair<meshID, IMesh*>(newMesh.getEntityID(), &newMesh));
	return newMesh.getEntityID();
}

textureID RenderingManager::addTextureData()
{
	//@TODO: dangling pointer problem here
	GLTexture newTexture;
	m_textureMap.emplace(std::pair<textureID, ITexture*>(newTexture.getEntityID(), &newTexture));
	return newTexture.getEntityID();
}

IMesh* RenderingManager::getMesh(meshID meshID)
{
	return m_meshMap.find(meshID)->second;
}

ITexture* RenderingManager::getTexture(textureID textureID)
{
	return m_textureMap.find(textureID)->second;
}

void RenderingManager::setup()
{
	m_childManager.emplace_back(&GLWindowManager::getInstance());
	m_childManager.emplace_back(&GLInputManager::getInstance());
	m_childManager.emplace_back(&GLRenderingManager::getInstance());
	//m_childEventManager.emplace_back(&GLGUIManager::getInstance());
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
	GLRenderingManager::getInstance().deferRender(SceneGraphManager::getInstance().getCameraQueue(), SceneGraphManager::getInstance().getLightQueue(), SceneGraphManager::getInstance().getRenderingQueue(), m_meshMap, m_textureMap);
	
	GLWindowManager::getInstance().update();

	GLInputManager::getInstance().update();
	//GLGUIManager::getInstance().update();

	//LogManager::getInstance().printLog("Async Rendering Finished.");
}
