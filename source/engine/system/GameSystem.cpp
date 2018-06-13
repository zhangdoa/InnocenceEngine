#include "GameSystem.h"

void GameSystem::setup()
{
	if (g_pGame)
	{
		g_pLogSystem->printLog("Game loaded.");
		g_pGame->setup();

		m_TransformComponents = g_pGame->getTransformComponents();
		m_VisibleComponents = g_pGame->getVisibleComponents();
		m_LightComponents = g_pGame->getLightComponents();
		m_CameraComponents = g_pGame->getCameraComponents();
		m_InputComponents = g_pGame->getInputComponents();

		std::for_each(m_TransformComponents.begin(), m_TransformComponents.end(), [&](TransformComponent* val)
		{
			m_TransformComponentsMap.emplace(val->getParentEntity(), val);
		});

		std::for_each(m_VisibleComponents.begin(), m_VisibleComponents.end(), [&](VisibleComponent* val)
		{
			m_VisibleComponentsMap.emplace(val->getParentEntity(), val);
		});

		std::for_each(m_LightComponents.begin(), m_LightComponents.end(), [&](LightComponent* val)
		{
			m_LightComponentsMap.emplace(val->getParentEntity(), val);
		});

		std::for_each(m_CameraComponents.begin(), m_CameraComponents.end(), [&](CameraComponent* val)
		{
			m_CameraComponentsMap.emplace(val->getParentEntity(), val);
		});

		std::for_each(m_InputComponents.begin(), m_InputComponents.end(), [&](InputComponent* val)
		{
			m_InputComponentsMap.emplace(val->getParentEntity(), val);
		});

		g_pLogSystem->printLog("Game setup finished.");
		m_objectStatus = objectStatus::ALIVE;
	}
	else
	{
		g_pLogSystem->printLog("No game loaded!");
		m_objectStatus = objectStatus::STANDBY;
	}
}

void GameSystem::initialize()
{
	g_pGame->initialize();
}

void GameSystem::update()
{
	auto l_tickTime = g_pTimeSystem->getcurrentTime();
	g_pGame->update();
	l_tickTime = g_pTimeSystem->getcurrentTime() - l_tickTime;
	//g_pLogSystem->printLog(l_tickTime);
}

void GameSystem::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}

std::vector<TransformComponent*>& GameSystem::getTransformComponents()
{
	return m_TransformComponents;
}

std::vector<VisibleComponent*>& GameSystem::getVisibleComponents()
{
	return m_VisibleComponents;
}

std::vector<LightComponent*>& GameSystem::getLightComponents()
{
	return m_LightComponents;
}

std::vector<CameraComponent*>& GameSystem::getCameraComponents()
{
	return m_CameraComponents;
}

std::vector<InputComponent*>& GameSystem::getInputComponents()
{
	return m_InputComponents;
}

TransformComponent * GameSystem::getTransformComponent(IEntity * parentEntity)
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

std::string GameSystem::getGameName() const
{
	return g_pGame->getGameName();
}

bool GameSystem::needRender()
{
	return m_needRender;
}

const objectStatus & GameSystem::getStatus() const
{
	return m_objectStatus;
}
