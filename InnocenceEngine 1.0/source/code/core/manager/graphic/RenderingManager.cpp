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

meshDataID RenderingManager::addMeshData()
{
	MeshData newMeshData;
	m_meshDatas.emplace(std::pair<meshDataID, MeshData>(newMeshData.getGameObjectID(), newMeshData));
	return newMeshData.getGameObjectID();
}

textureDataID RenderingManager::addTextureData()
{
	TextureData newTextureData;
	m_textureDatas.emplace(std::pair<textureDataID, TextureData>(newTextureData.getGameObjectID(), newTextureData));
	return newTextureData.getGameObjectID();
}

std::unordered_map<meshDataID, MeshData>& RenderingManager::getMeshData()
{
	return m_meshDatas;
}

std::unordered_map<textureDataID, TextureData>& RenderingManager::getTextureData()
{
	return m_textureDatas;
}

MeshData & RenderingManager::getMeshData(meshDataID meshDataIndex)
{
	return m_meshDatas.find(meshDataIndex)->second;
}

TextureData & RenderingManager::getTextureData(textureDataID textureDataIndex)
{
	return m_textureDatas.find(textureDataIndex)->second;
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
		m_childEventManager[i].get()->initialize();
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
		m_childEventManager[m_childEventManager.size() - 1 - i].get()->shutdown();
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
	GLRenderingManager::getInstance().update();

	//forward render
	//GLRenderingManager::getInstance().forwardRender(SceneGraphManager::getInstance().getCameraQueue(), SceneGraphManager::getInstance().getLightQueue(), SceneGraphManager::getInstance().getRenderingQueue());
	
	//defer render
	GLRenderingManager::getInstance().deferRender(SceneGraphManager::getInstance().getCameraQueue(), SceneGraphManager::getInstance().getLightQueue(), SceneGraphManager::getInstance().getRenderingQueue(), m_meshDatas, m_textureDatas);
	
	GLWindowManager::getInstance().update();

	GLInputManager::getInstance().update();
	//GLGUIManager::getInstance().update();

	//LogManager::getInstance().printLog("Async Rendering Finished.");
}
