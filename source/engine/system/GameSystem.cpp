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

void GameSystem::addMeshData(VisibleComponent * visibleComponentconst, meshID & meshID)
{
	visibleComponentconst->m_modelMap.emplace(meshID, textureMap());
}

void GameSystem::addTextureData(VisibleComponent * visibleComponentconst, const texturePair & texturePair)
{
	for (auto& l_model : visibleComponentconst->m_modelMap)
	{
		auto l_texturePair = l_model.second.find(texturePair.first);
		if (l_texturePair == l_model.second.end())
		{
			l_model.second.emplace(texturePair);
		}
	}
}

void GameSystem::overwriteTextureData(VisibleComponent * visibleComponentconst, const texturePair & texturePair)
{
	for (auto& l_model : visibleComponentconst->m_modelMap)
	{
		auto l_texturePair = l_model.second.find(texturePair.first);
		if (l_texturePair == l_model.second.end())
		{
			l_model.second.emplace(texturePair);
		}
		else
		{
			l_texturePair->second = texturePair.second;
		}
	}
}

mat4 GameSystem::getProjectionMatrix(LightComponent * lightComponent, unsigned int cascadedLevel)
{
	auto l_center = lightComponent->m_AABBs[cascadedLevel].m_center;
	double l_radius = 0.0;
	for (unsigned int i = 0; i < 8; ++i)
	{
		auto l_vertex = lightComponent->m_AABBs[cascadedLevel].m_vertices[i];

		double l_distance = (l_vertex.m_pos - l_center).length();
		l_radius = std::max(l_radius, l_distance);
	}
	vec4 l_maxExtents = vec4(l_radius, l_radius, l_radius, 1.0);
	vec4 l_minExtents = l_maxExtents * (-1.0);
	vec4 l_cascadeExtents = l_maxExtents - l_minExtents;
	mat4 p;
	p.initializeToOrthographicMatrix(l_minExtents.x, l_maxExtents.x, l_minExtents.y, l_maxExtents.y, l_minExtents.z, l_maxExtents.z);
	return p;
}

void GameSystem::registerKeyboardInputCallback(InputComponent * inputComponent, int keyCode, std::function<void()>* function)
{
	auto l_keyboardInputCallbackVector = inputComponent->m_keyboardInputCallbackImpl.find(keyCode);
	if (l_keyboardInputCallbackVector != inputComponent->m_keyboardInputCallbackImpl.end())
	{
		l_keyboardInputCallbackVector->second.emplace_back(function);
	}
	else
	{
		inputComponent->m_keyboardInputCallbackImpl.emplace(keyCode, std::vector<std::function<void()>*>{function});
	}
}

void GameSystem::registerMouseInputCallback(InputComponent * inputComponent, int mouseCode, std::function<void(double)>* function)
{
	auto l_mouseInputCallbackVector = inputComponent->m_mouseMovementCallbackImpl.find(mouseCode);
	if (l_mouseInputCallbackVector != inputComponent->m_mouseMovementCallbackImpl.end())
	{
		l_mouseInputCallbackVector->second.emplace_back(function);
	}
	else
	{
		inputComponent->m_mouseMovementCallbackImpl.emplace(mouseCode, std::vector<std::function<void(double)>*>{function});
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
