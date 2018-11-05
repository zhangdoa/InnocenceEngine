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
	void sortTransformComponentsVector();

	void updateTransform();

	objectStatus m_GameSystemStatus = objectStatus::SHUTDOWN;

	static GameSystemSingletonComponent* g_GameSystemSingletonComponent;
}

InnoHighLevelSystem_EXPORT bool InnoGameSystem::setup()
{
	g_GameSystemSingletonComponent = &GameSystemSingletonComponent::getInstance();
	InnoGameInstance::setup();
	sortTransformComponentsVector();
	m_GameSystemStatus = objectStatus::ALIVE;
	return true;
}

void InnoGameSystem::sortTransformComponentsVector()
{
	//construct the hierarchy tree
	std::for_each(g_GameSystemSingletonComponent->m_transformComponents.begin(), g_GameSystemSingletonComponent->m_transformComponents.end(), [&](TransformComponent* val)
	{
		if (val->m_parentTransformComponent)
		{
			val->m_transformHierarchyLevel = val->m_parentTransformComponent->m_transformHierarchyLevel + 1;
		}
		else
		{
			val->m_transformHierarchyLevel = 0;
		}
	});
	//from top to bottom
	std::sort(g_GameSystemSingletonComponent->m_transformComponents.begin(), g_GameSystemSingletonComponent->m_transformComponents.end(), [&](TransformComponent* a, TransformComponent* b)
	{
		return a->m_transformHierarchyLevel < b->m_transformHierarchyLevel;
	});
}

void InnoGameSystem::updateTransform()
{
	// @TODO: update from hierarchy's top to down
	std::for_each(g_GameSystemSingletonComponent->m_transformComponents.begin(), g_GameSystemSingletonComponent->m_transformComponents.end(), [&](TransformComponent* val)
	{
		val->m_localTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_localTransformVector);
		val->m_globalTransformVector = InnoMath::LocalTransformVectorToGlobal(val->m_localTransformVector, val->m_parentTransformComponent->m_globalTransformVector, val->m_parentTransformComponent->m_globalTransformMatrix);
		val->m_globalTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_globalTransformVector);
	});
}

// @TODO: add a cache function for after-rendering business
void InnoGameSystem::saveComponentsCapture()
{
	std::for_each(g_GameSystemSingletonComponent->m_transformComponents.begin(), g_GameSystemSingletonComponent->m_transformComponents.end(), [&](TransformComponent* val)
	{
		val->m_globalTransformMatrix_prev = val->m_globalTransformMatrix;
	});
}

InnoHighLevelSystem_EXPORT bool InnoGameSystem::initialize()
{
	InnoGameInstance::initialize();
	InnoLogSystem::printLog("GameSystem has been initialized.");
	return true;
}

InnoHighLevelSystem_EXPORT bool InnoGameSystem::update()
{
	g_GameSystemSingletonComponent->m_asyncTask = &InnoTaskSystem::submit([]()
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
	g_GameSystemSingletonComponent->m_transformComponents.emplace_back(transformComponent);
	g_GameSystemSingletonComponent->m_TransformComponentsMap.emplace(transformComponent->m_parentEntity, transformComponent);
}

void InnoGameSystem::registerComponents(VisibleComponent * visibleComponent)
{
	g_GameSystemSingletonComponent->m_visibleComponents.emplace_back(visibleComponent);
	g_GameSystemSingletonComponent->m_VisibleComponentsMap.emplace(visibleComponent->m_parentEntity, visibleComponent);
}

void InnoGameSystem::registerComponents(LightComponent * lightComponent)
{
	g_GameSystemSingletonComponent->m_lightComponents.emplace_back(lightComponent);
	g_GameSystemSingletonComponent->m_LightComponentsMap.emplace(lightComponent->m_parentEntity, lightComponent);
}

void InnoGameSystem::registerComponents(CameraComponent * cameraComponent)
{
	g_GameSystemSingletonComponent->m_cameraComponents.emplace_back(cameraComponent);
	g_GameSystemSingletonComponent->m_CameraComponentsMap.emplace(cameraComponent->m_parentEntity, cameraComponent);
}

void InnoGameSystem::registerComponents(InputComponent * inputComponent)
{
	g_GameSystemSingletonComponent->m_inputComponents.emplace_back(inputComponent);
	g_GameSystemSingletonComponent->m_InputComponentsMap.emplace(inputComponent->m_parentEntity, inputComponent);
}

void InnoGameSystem::registerComponents(EnvironmentCaptureComponent * environmentCaptureComponent)
{
	g_GameSystemSingletonComponent->m_environmentCaptureComponents.emplace_back(environmentCaptureComponent);
	g_GameSystemSingletonComponent->m_EnvironmentCaptureComponentsMap.emplace(environmentCaptureComponent->m_parentEntity, environmentCaptureComponent);
}

void InnoGameSystem::registerComponents(PlayerComponent * playerComponent)
{
	registerComponents(playerComponent->m_transformComponent);
	registerComponents(playerComponent->m_visibleComponent);
	registerComponents(playerComponent->m_inputComponent);
	registerComponents(playerComponent->m_cameraComponent);
}

std::string InnoGameSystem::getGameName()
{
	return InnoGameInstance::getGameName();
}

// @TODO: generic impl
TransformComponent * InnoGameSystem::getTransformComponent(EntityID parentEntity)
{
	auto result = g_GameSystemSingletonComponent->m_TransformComponentsMap.find(parentEntity);
	if (result != g_GameSystemSingletonComponent->m_TransformComponentsMap.end())
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

void InnoGameSystem::registerMouseMovementCallback(InputComponent * inputComponent, int mouseCode, std::function<void(float)>* function)
{
	auto l_mouseMovementCallbackVector = inputComponent->m_mouseMovementCallbackImpl.find(mouseCode);
	if (l_mouseMovementCallbackVector != inputComponent->m_mouseMovementCallbackImpl.end())
	{
		l_mouseMovementCallbackVector->second.emplace_back(function);
	}
	else
	{
		inputComponent->m_mouseMovementCallbackImpl.emplace(mouseCode, std::vector<std::function<void(float)>*>{function});
	}
}

InnoHighLevelSystem_EXPORT objectStatus InnoGameSystem::getStatus()
{
	return m_GameSystemStatus;
}
