#include "GameSystem.h"

void GameSystem::setup()
{
		m_objectStatus = objectStatus::ALIVE;
}

void GameSystem::addComponentsToMap()
{
	std::for_each(m_transformComponents.begin(), m_transformComponents.end(), [&](TransformComponent* val)
	{
		m_TransformComponentsMap.emplace(val->getParentEntity(), val);
	});
	std::for_each(m_visibleComponents.begin(), m_visibleComponents.end(), [&](VisibleComponent* val)
	{
		m_VisibleComponentsMap.emplace(val->getParentEntity(), val);
	});

	std::for_each(m_lightComponents.begin(), m_lightComponents.end(), [&](LightComponent* val)
	{
		m_LightComponentsMap.emplace(val->getParentEntity(), val);
	});

	std::for_each(m_cameraComponents.begin(), m_cameraComponents.end(), [&](CameraComponent* val)
	{
		m_CameraComponentsMap.emplace(val->getParentEntity(), val);
	});

	std::for_each(m_inputComponents.begin(), m_inputComponents.end(), [&](InputComponent* val)
	{
		m_InputComponentsMap.emplace(val->getParentEntity(), val);
	});
}

void GameSystem::initialize()
{
}

void GameSystem::update()
{
	//auto l_tickTime = g_pTimeSystem->getcurrentTime();
	//l_tickTime = g_pTimeSystem->getcurrentTime() - l_tickTime;
	////g_pLogSystem->printLog(l_tickTime);
}

void GameSystem::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}

std::vector<TransformComponent*>& GameSystem::getTransformComponents()
{
	return m_transformComponents;
}

std::vector<VisibleComponent*>& GameSystem::getVisibleComponents()
{
	return m_visibleComponents;
}

std::vector<LightComponent*>& GameSystem::getLightComponents()
{
	return m_lightComponents;
}

std::vector<CameraComponent*>& GameSystem::getCameraComponents()
{
	return m_cameraComponents;
}

std::vector<InputComponent*>& GameSystem::getInputComponents()
{
	return m_inputComponents;
}

TransformComponent * GameSystem::getTransformComponent(EntityID parentEntity)
{
	auto result = m_TransformComponentsMap.find(parentEntity);
	if (result != m_TransformComponentsMap.end())
	{
		return result->second;
	}
	else
	{
		return nullptr;
	}
}

bool GameSystem::needRender()
{
	return m_needRender;
}

EntityID GameSystem::createEntityID()
{
	return std::rand();
}

const objectStatus & GameSystem::getStatus() const
{
	return m_objectStatus;
}
