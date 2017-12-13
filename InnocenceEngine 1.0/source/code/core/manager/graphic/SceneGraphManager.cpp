#include "../../../main/stdafx.h"
#include "SceneGraphManager.h"

SceneGraphManager::SceneGraphManager()
{
}


SceneGraphManager::~SceneGraphManager()
{
}


void SceneGraphManager::addToRenderingQueue(VisibleComponent* visibleComponent)
{
	m_visibleComponents.emplace_back(visibleComponent);
}

void SceneGraphManager::addToLightQueue(LightComponent * lightComponent)
{
	m_LightComponents.emplace_back(lightComponent);
}

void SceneGraphManager::addToCameraQueue(CameraComponent * cameraComponent)
{
	m_CameraComponents.emplace_back(cameraComponent);
}

void SceneGraphManager::addToInputQueue(InputComponent * inputComponent)
{
	m_InputComponents.emplace_back(inputComponent);
}

std::vector<VisibleComponent*>& SceneGraphManager::getRenderingQueue()
{
	return m_visibleComponents;
}

std::vector<LightComponent*>& SceneGraphManager::getLightQueue()
{
	return m_LightComponents;
}

std::vector<CameraComponent*>& SceneGraphManager::getCameraQueue()
{
	return m_CameraComponents;
}

std::vector<InputComponent*>& SceneGraphManager::getInputQueue()
{
	return m_InputComponents;
}


void SceneGraphManager::setup()
{
}

void SceneGraphManager::initialize()
{
	this->setStatus(objectStatus::ALIVE);
	LogManager::getInstance().printLog("SceneGraphManager has been initialized.");
}

void SceneGraphManager::update()
{
}

void SceneGraphManager::shutdown()
{
	m_visibleComponents.empty();
	m_LightComponents.empty();
	m_CameraComponents.empty();
	m_InputComponents.empty();
	this->setStatus(objectStatus::SHUTDOWN);
	LogManager::getInstance().printLog("SceneGraphManager has been shutdown.");
}
