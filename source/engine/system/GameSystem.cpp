#include "GameSystem.h"
#include "../common/config.h"

#if defined (BUILD_EDITOR)
#include "../../game/InnocenceEditor/InnocenceEditor.h"
#define InnoGameInstance InnocenceEditor
#elif defined (BUILD_GAME)
#include "../../game/InnocenceGarden/InnocenceGarden.h"
#define InnoGameInstance InnocenceGarden
#elif defined (BUILD_TEST)
#include "../../game/InnocenceTest/InnocenceTest.h"
#define InnoGameInstance InnocenceTest
#endif

namespace InnoGameSystem
{
	// the SOA here
	std::vector<TransformComponent*> m_transformComponents;
	std::vector<VisibleComponent*> m_visibleComponents;
	std::vector<LightComponent*> m_lightComponents;
	std::vector<CameraComponent*> m_cameraComponents;
	std::vector<InputComponent*> m_inputComponents;

	std::unordered_map<EntityID, TransformComponent*> m_TransformComponentsMap;
	std::unordered_multimap<EntityID, VisibleComponent*> m_VisibleComponentsMap;
	std::unordered_multimap<EntityID, LightComponent*> m_LightComponentsMap;
	std::unordered_multimap<EntityID, CameraComponent*> m_CameraComponentsMap;
	std::unordered_multimap<EntityID, InputComponent*> m_InputComponentsMap;

	bool m_needRender = true;

	objectStatus m_GameSystemStatus = objectStatus::SHUTDOWN;
}

void InnoGameSystem::setup()
{
	InnoGameInstance::setup();
	addComponentsToMap();
	m_GameSystemStatus = objectStatus::ALIVE;
}

void InnoGameSystem::addComponentsToMap()
{
	std::for_each(m_transformComponents.begin(), m_transformComponents.end(), [&](TransformComponent* val)
	{
		m_TransformComponentsMap.emplace(val->m_parentEntity, val);
	});
	std::for_each(m_visibleComponents.begin(), m_visibleComponents.end(), [&](VisibleComponent* val)
	{
		m_VisibleComponentsMap.emplace(val->m_parentEntity, val);
	});

	std::for_each(m_lightComponents.begin(), m_lightComponents.end(), [&](LightComponent* val)
	{
		m_LightComponentsMap.emplace(val->m_parentEntity, val);
	});

	std::for_each(m_cameraComponents.begin(), m_cameraComponents.end(), [&](CameraComponent* val)
	{
		m_CameraComponentsMap.emplace(val->m_parentEntity, val);
	});

	std::for_each(m_inputComponents.begin(), m_inputComponents.end(), [&](InputComponent* val)
	{
		m_InputComponentsMap.emplace(val->m_parentEntity, val);
	});
}

void InnoGameSystem::initialize()
{
	InnoGameInstance::initialize();
}

void InnoGameSystem::update()
{
	InnoGameInstance::update();
	//auto l_tickTime = InnoTimeSystem->getcurrentTime();
	//l_tickTime = InnoTimeSystem->getcurrentTime() - l_tickTime;
	////InnoLogSystem::printLog(l_tickTime);
}

void InnoGameSystem::shutdown()
{
	InnoGameInstance::shutdown();
	m_GameSystemStatus = objectStatus::SHUTDOWN;
}

void InnoGameSystem::addTransformComponent(TransformComponent * rhs)
{
	m_transformComponents.emplace_back(rhs);
}

void InnoGameSystem::addVisibleComponent(VisibleComponent * rhs)
{
	m_visibleComponents.emplace_back(rhs);
}

void InnoGameSystem::addLightComponent(LightComponent * rhs)
{
	m_lightComponents.emplace_back(rhs);
}

void InnoGameSystem::addCameraComponent(CameraComponent * rhs)
{
	m_cameraComponents.emplace_back(rhs);
}

void InnoGameSystem::addInputComponent(InputComponent * rhs)
{
	m_inputComponents.emplace_back(rhs);
}

std::vector<TransformComponent*>& InnoGameSystem::getTransformComponents()
{
	return m_transformComponents;
}

std::vector<VisibleComponent*>& InnoGameSystem::getVisibleComponents()
{
	return m_visibleComponents;
}

std::vector<LightComponent*>& InnoGameSystem::getLightComponents()
{
	return m_lightComponents;
}

std::vector<CameraComponent*>& InnoGameSystem::getCameraComponents()
{
	return m_cameraComponents;
}

std::vector<InputComponent*>& InnoGameSystem::getInputComponents()
{
	return m_inputComponents;
}

std::string InnoGameSystem::getGameName()
{
	return InnoGameInstance::getGameName();
}

TransformComponent * InnoGameSystem::getTransformComponent(EntityID parentEntity)
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

void InnoGameSystem::addMeshData(VisibleComponent * visibleComponentconst, meshID & meshID)
{
	visibleComponentconst->m_modelMap.emplace(meshID, textureMap());
}

void InnoGameSystem::addTextureData(VisibleComponent * visibleComponentconst, const texturePair & texturePair)
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

void InnoGameSystem::overwriteTextureData(VisibleComponent * visibleComponentconst, const texturePair & texturePair)
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

mat4 InnoGameSystem::getProjectionMatrix(LightComponent * lightComponent, unsigned int cascadedLevel)
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

void InnoGameSystem::registerButtonStatusCallback(InputComponent * inputComponent, button boundButton, std::function<void()>* function)
{
	auto l_kbuttonStatusCallbackVector = inputComponent->m_buttonStatusCallbackImpl.find(boundButton);
	if (l_kbuttonStatusCallbackVector != inputComponent->m_buttonStatusCallbackImpl.end())
	{
		l_kbuttonStatusCallbackVector->second.emplace_back(function);
	}
	else
	{
		inputComponent->m_buttonStatusCallbackImpl.emplace(boundButton, std::vector<std::function<void()>*>{function});
	}
}

void InnoGameSystem::registerMouseMovementCallback(InputComponent * inputComponent, int mouseCode, std::function<void(double)>* function)
{
	auto l_mouseMovementCallbackVector = inputComponent->m_mouseMovementCallbackImpl.find(mouseCode);
	if (l_mouseMovementCallbackVector != inputComponent->m_mouseMovementCallbackImpl.end())
	{
		l_mouseMovementCallbackVector->second.emplace_back(function);
	}
	else
	{
		inputComponent->m_mouseMovementCallbackImpl.emplace(mouseCode, std::vector<std::function<void(double)>*>{function});
	}
}

bool InnoGameSystem::needRender()
{
	return m_needRender;
}

EntityID InnoGameSystem::createEntityID()
{
	return std::rand();
}

objectStatus InnoGameSystem::getStatus()
{
	return m_GameSystemStatus;
}
