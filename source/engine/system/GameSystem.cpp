#include "GameSystem.h"
#include "../common/config.h"
#include "../component/GameSystemSingletonComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoGameSystemNS
{
	void sortTransformComponentsVector();

	void updateTransform();

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	IGameInstance* m_GameInstance;
	static GameSystemSingletonComponent* g_GameSystemSingletonComponent;
}

INNO_SYSTEM_EXPORT bool InnoGameSystem::setup()
{
	InnoGameSystemNS::g_GameSystemSingletonComponent = &GameSystemSingletonComponent::getInstance();
	InnoGameSystemNS::m_GameInstance->setup();
	InnoGameSystemNS::sortTransformComponentsVector();
	InnoGameSystemNS::m_objectStatus = objectStatus::ALIVE;
	return true;
}

void InnoGameSystemNS::sortTransformComponentsVector()
{
	//construct the hierarchy tree
	std::for_each(InnoGameSystemNS::g_GameSystemSingletonComponent->m_TransformComponents.begin(), g_GameSystemSingletonComponent->m_TransformComponents.end(), [&](TransformComponent* val)
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
	std::sort(g_GameSystemSingletonComponent->m_TransformComponents.begin(), g_GameSystemSingletonComponent->m_TransformComponents.end(), [&](TransformComponent* a, TransformComponent* b)
	{
		return a->m_transformHierarchyLevel < b->m_transformHierarchyLevel;
	});
}

void InnoGameSystemNS::updateTransform()
{
	// @TODO: update from hierarchy's top to down
	std::for_each(InnoGameSystemNS::g_GameSystemSingletonComponent->m_TransformComponents.begin(), InnoGameSystemNS::g_GameSystemSingletonComponent->m_TransformComponents.end(), [&](TransformComponent* val)
	{
		val->m_localTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_localTransformVector);
		val->m_globalTransformVector = InnoMath::LocalTransformVectorToGlobal(val->m_localTransformVector, val->m_parentTransformComponent->m_globalTransformVector, val->m_parentTransformComponent->m_globalTransformMatrix);
		val->m_globalTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_globalTransformVector);
	});
}

// @TODO: add a cache function for after-rendering business
INNO_SYSTEM_EXPORT void InnoGameSystem::saveComponentsCapture()
{
	std::for_each(InnoGameSystemNS::g_GameSystemSingletonComponent->m_TransformComponents.begin(), InnoGameSystemNS::g_GameSystemSingletonComponent->m_TransformComponents.end(), [&](TransformComponent* val)
	{
		val->m_globalTransformMatrix_prev = val->m_globalTransformMatrix;
	});
}

INNO_SYSTEM_EXPORT void InnoGameSystem::setGameInstance(IGameInstance * gameInstance)
{
	InnoGameSystemNS::m_GameInstance = gameInstance;
}

INNO_SYSTEM_EXPORT bool InnoGameSystem::initialize()
{
	InnoGameSystemNS::m_GameInstance->initialize();
	g_pCoreSystem->getLogSystem()->printLog("GameSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoGameSystem::update()
{
	InnoGameSystemNS::g_GameSystemSingletonComponent->m_asyncTask = &g_pCoreSystem->getTaskSystem()->submit([]()
	{
		InnoGameSystemNS::m_GameInstance->update();
		InnoGameSystemNS::updateTransform();
	});
	return true;
}

INNO_SYSTEM_EXPORT bool InnoGameSystem::terminate()
{
	InnoGameSystemNS::m_GameInstance->terminate();
	InnoGameSystemNS::m_objectStatus = objectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog("GameSystem has been terminated.");
	return true;
}

#define spawnComponentImplDefi( className ) \
INNO_SYSTEM_EXPORT void InnoGameSystem::spawnComponent(className* rhs) \
{ \
	InnoGameSystemNS::g_GameSystemSingletonComponent->m_##className##s.emplace_back(rhs); \
	InnoGameSystemNS::g_GameSystemSingletonComponent->m_##className##sMap.emplace(rhs->m_parentEntity, rhs); \
}

spawnComponentImplDefi(TransformComponent)
spawnComponentImplDefi(VisibleComponent)
spawnComponentImplDefi(LightComponent)
spawnComponentImplDefi(CameraComponent)
spawnComponentImplDefi(InputComponent)
spawnComponentImplDefi(EnvironmentCaptureComponent)

INNO_SYSTEM_EXPORT std::string InnoGameSystem::getGameName()
{
	return InnoGameSystemNS::m_GameInstance->getGameName();
}

#define getComponentImplDefi( className ) \
INNO_SYSTEM_EXPORT className* InnoGameSystem::get##className(EntityID parentEntity) \
{ \
	auto result = InnoGameSystemNS::g_GameSystemSingletonComponent->m_##className##sMap.find(parentEntity); \
	if (result != InnoGameSystemNS::g_GameSystemSingletonComponent->m_##className##sMap.end()) \
	{ \
		return result->second; \
	} \
	else \
	{ \
		return nullptr; \
	} \
}

getComponentImplDefi(TransformComponent)
getComponentImplDefi(VisibleComponent)
getComponentImplDefi(LightComponent)
getComponentImplDefi(CameraComponent)
getComponentImplDefi(InputComponent)
getComponentImplDefi(EnvironmentCaptureComponent)

INNO_SYSTEM_EXPORT void InnoGameSystem::registerButtonStatusCallback(InputComponent * inputComponent, button boundButton, std::function<void()>* function)
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

INNO_SYSTEM_EXPORT void InnoGameSystem::registerMouseMovementCallback(InputComponent * inputComponent, int mouseCode, std::function<void(float)>* function)
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

INNO_SYSTEM_EXPORT objectStatus InnoGameSystem::getStatus()
{
	return InnoGameSystemNS::m_objectStatus;
}
