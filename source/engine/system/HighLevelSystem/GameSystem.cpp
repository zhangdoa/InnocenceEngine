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
	void updateTransform();

	objectStatus m_GameSystemStatus = objectStatus::SHUTDOWN;
}

InnoHighLevelSystem_EXPORT bool InnoGameSystem::setup()
{
	InnoGameInstance::setup();
	m_GameSystemStatus = objectStatus::ALIVE;
	return true;
}

void InnoGameSystem::updateTransform()
{
	// @TODO: update from hierarchy's top to down
	std::for_each(GameSystemSingletonComponent::getInstance().m_currentTransformComponentsTree.begin(), GameSystemSingletonComponent::getInstance().m_currentTransformComponentsTree.end(), [&](std::vector<TransformComponent*> vector)
	{
		std::for_each(vector.begin(), vector.end(), [&](TransformComponent* val)
		{
			val->m_localTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_localTransformVector);
			val->m_globalTransformVector = InnoMath::LocalTransformVectorToGlobal(val->m_parentTransformComponent->m_globalTransformVector, val->m_parentTransformComponent->m_globalTransformMatrix);
			val->m_globalTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_globalTransformVector);
		});
	});
}

// @TODO: add a cache function for after-rendering business
void InnoGameSystem::saveComponentsCapture()
{
	GameSystemSingletonComponent::getInstance().m_previousTransformComponentsTree = GameSystemSingletonComponent::getInstance().m_currentTransformComponentsTree;
}

InnoHighLevelSystem_EXPORT bool InnoGameSystem::initialize()
{
	InnoGameInstance::initialize();
	InnoLogSystem::printLog("GameSystem has been initialized.");
	return true;
}

InnoHighLevelSystem_EXPORT bool InnoGameSystem::update()
{
	GameSystemSingletonComponent::getInstance().m_asyncTask = &InnoTaskSystem::submit([]()
	{
		InnoGameInstance::update();
		updateTransform();
	});
	return true;
}

InnoHighLevelSystem_EXPORT bool InnoGameSystem::terminate()
{
	InnoGameInstance::terminate();
	m_GameSystemStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("GameSystem has been terminated.");
	return true;
}

void InnoGameSystem::registerComponents(TransformComponent * transformComponent)
{
	GameSystemSingletonComponent::getInstance().m_transformComponents.emplace_back(transformComponent);
	GameSystemSingletonComponent::getInstance().m_TransformComponentsMap.emplace(transformComponent->m_parentEntity, transformComponent);
}

void InnoGameSystem::registerComponents(VisibleComponent * visibleComponent)
{
	GameSystemSingletonComponent::getInstance().m_visibleComponents.emplace_back(visibleComponent);
	GameSystemSingletonComponent::getInstance().m_VisibleComponentsMap.emplace(visibleComponent->m_parentEntity, visibleComponent);
}

void InnoGameSystem::registerComponents(LightComponent * lightComponent)
{
	GameSystemSingletonComponent::getInstance().m_lightComponents.emplace_back(lightComponent);
	GameSystemSingletonComponent::getInstance().m_LightComponentsMap.emplace(lightComponent->m_parentEntity, lightComponent);
}

void InnoGameSystem::registerComponents(CameraComponent * cameraComponent)
{
	GameSystemSingletonComponent::getInstance().m_cameraComponents.emplace_back(cameraComponent);
	GameSystemSingletonComponent::getInstance().m_CameraComponentsMap.emplace(cameraComponent->m_parentEntity, cameraComponent);
}

void InnoGameSystem::registerComponents(InputComponent * inputComponent)
{
	GameSystemSingletonComponent::getInstance().m_inputComponents.emplace_back(inputComponent);
	GameSystemSingletonComponent::getInstance().m_InputComponentsMap.emplace(inputComponent->m_parentEntity, inputComponent);
}

void InnoGameSystem::registerComponents(EnvironmentCaptureComponent * environmentCaptureComponent)
{
	GameSystemSingletonComponent::getInstance().m_environmentCaptureComponents.emplace_back(environmentCaptureComponent);
	GameSystemSingletonComponent::getInstance().m_EnvironmentCaptureComponentsMap.emplace(environmentCaptureComponent->m_parentEntity, environmentCaptureComponent);
}

std::string InnoGameSystem::getGameName()
{
	return InnoGameInstance::getGameName();
}

// @TODO: generic impl
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

InnoHighLevelSystem_EXPORT objectStatus InnoGameSystem::getStatus()
{
	return m_GameSystemStatus;
}
