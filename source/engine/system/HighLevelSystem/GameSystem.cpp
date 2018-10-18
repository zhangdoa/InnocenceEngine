#include "GameSystem.h"
#include "../../common/config.h"
#include "../../component/GameSystemSingletonComponent.h"

#include "../LowLevelSystem/TaskSystem.h"
#include "../LowLevelSystem/LogSystem.h"

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
	void addComponentsToMap();

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
	std::for_each(GameSystemSingletonComponent::getInstance().m_transformComponents.begin(), GameSystemSingletonComponent::getInstance().m_transformComponents.end(), [&](TransformComponent* val)
	{
		GameSystemSingletonComponent::getInstance().m_TransformComponentsMap.emplace(val->m_parentEntity, val);
	});
	std::for_each(GameSystemSingletonComponent::getInstance().m_visibleComponents.begin(), GameSystemSingletonComponent::getInstance().m_visibleComponents.end(), [&](VisibleComponent* val)
	{
		GameSystemSingletonComponent::getInstance().m_VisibleComponentsMap.emplace(val->m_parentEntity, val);
	});

	std::for_each(GameSystemSingletonComponent::getInstance().m_lightComponents.begin(), GameSystemSingletonComponent::getInstance().m_lightComponents.end(), [&](LightComponent* val)
	{
		GameSystemSingletonComponent::getInstance().m_LightComponentsMap.emplace(val->m_parentEntity, val);
	});

	std::for_each(GameSystemSingletonComponent::getInstance().m_cameraComponents.begin(), GameSystemSingletonComponent::getInstance().m_cameraComponents.end(), [&](CameraComponent* val)
	{
		GameSystemSingletonComponent::getInstance().m_CameraComponentsMap.emplace(val->m_parentEntity, val);
	});

	std::for_each(GameSystemSingletonComponent::getInstance().m_inputComponents.begin(), GameSystemSingletonComponent::getInstance().m_inputComponents.end(), [&](InputComponent* val)
	{
		GameSystemSingletonComponent::getInstance().m_InputComponentsMap.emplace(val->m_parentEntity, val);
	});

	std::for_each(GameSystemSingletonComponent::getInstance().m_environmentCaptureComponents.begin(), GameSystemSingletonComponent::getInstance().m_environmentCaptureComponents.end(), [&](EnvironmentCaptureComponent* val)
	{
		GameSystemSingletonComponent::getInstance().m_EnvironmentCaptureComponentsMap.emplace(val->m_parentEntity, val);
	});
}

void InnoGameSystem::initialize()
{
	InnoGameInstance::initialize();
	InnoLogSystem::printLog("GameSystem has been initialized.");
}

void InnoGameSystem::update()
{
	GameSystemSingletonComponent::getInstance().m_asyncTask = &InnoTaskSystem::submit([]()
	{
		InnoGameInstance::update();
	});
}

void InnoGameSystem::shutdown()
{
	InnoGameInstance::shutdown();
	m_GameSystemStatus = objectStatus::SHUTDOWN;
}

void InnoGameSystem::addTransformComponent(TransformComponent * rhs)
{
	GameSystemSingletonComponent::getInstance().m_transformComponents.emplace_back(rhs);
}

void InnoGameSystem::addVisibleComponent(VisibleComponent * rhs)
{
	GameSystemSingletonComponent::getInstance().m_visibleComponents.emplace_back(rhs);
}

void InnoGameSystem::addLightComponent(LightComponent * rhs)
{
	GameSystemSingletonComponent::getInstance().m_lightComponents.emplace_back(rhs);
}

void InnoGameSystem::addCameraComponent(CameraComponent * rhs)
{
	GameSystemSingletonComponent::getInstance().m_cameraComponents.emplace_back(rhs);
}

void InnoGameSystem::addInputComponent(InputComponent * rhs)
{
	GameSystemSingletonComponent::getInstance().m_inputComponents.emplace_back(rhs);
}

void InnoGameSystem::addEnvironmentCaptureComponent(EnvironmentCaptureComponent * rhs)
{
	GameSystemSingletonComponent::getInstance().m_environmentCaptureComponents.emplace_back(rhs);
}

std::string InnoGameSystem::getGameName()
{
	return InnoGameInstance::getGameName();
}

TransformComponent * InnoGameSystem::getTransformComponent(EntityID parentEntity)
{
	auto result = GameSystemSingletonComponent::getInstance().m_TransformComponentsMap.find(parentEntity);
	if (result != GameSystemSingletonComponent::getInstance().m_TransformComponentsMap.end())
	{
		return result->second;
	}
	else
	{
		return nullptr;
	}
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

EntityID InnoGameSystem::createEntityID()
{
	return std::rand();
}

objectStatus InnoGameSystem::getStatus()
{
	return m_GameSystemStatus;
}
