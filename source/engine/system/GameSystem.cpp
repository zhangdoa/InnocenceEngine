#include "GameSystem.h"
#include "../common/config.h"
#include "../component/GameSystemComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoGameSystemNS
{
	void sortTransformComponentsVector();

	void updateTransform();

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	static GameSystemComponent* g_GameSystemComponent;
	IGameInstance* m_gameInstance;
}

INNO_SYSTEM_EXPORT bool InnoGameSystem::setup()
{
	InnoGameSystemNS::g_GameSystemComponent = &GameSystemComponent::get();

	// setup root TransformComponent
	InnoGameSystemNS::g_GameSystemComponent->m_rootTransformComponent = new TransformComponent();
	InnoGameSystemNS::g_GameSystemComponent->m_rootTransformComponent->m_parentTransformComponent = nullptr;

	InnoGameSystemNS::g_GameSystemComponent->m_rootTransformComponent->m_parentEntity = InnoMath::createEntityID();

	InnoGameSystemNS::g_GameSystemComponent->m_rootTransformComponent->m_localTransformMatrix = InnoMath::TransformVectorToTransformMatrix(InnoGameSystemNS::g_GameSystemComponent->m_rootTransformComponent->m_localTransformVector);
	InnoGameSystemNS::g_GameSystemComponent->m_rootTransformComponent->m_globalTransformVector = InnoGameSystemNS::g_GameSystemComponent->m_rootTransformComponent->m_localTransformVector;
	InnoGameSystemNS::g_GameSystemComponent->m_rootTransformComponent->m_globalTransformMatrix = InnoGameSystemNS::g_GameSystemComponent->m_rootTransformComponent->m_localTransformMatrix;

	InnoGameSystemNS::m_objectStatus = ObjectStatus::ALIVE;

	if (!InnoGameSystemNS::m_gameInstance->setup())
	{
		return false;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameInstance setup finished.");

	return true;
}

void InnoGameSystemNS::sortTransformComponentsVector()
{
	//construct the hierarchy tree
	for (auto i : InnoGameSystemNS::g_GameSystemComponent->m_TransformComponents)
	{
		if (i->m_parentTransformComponent)
		{
			i->m_transformHierarchyLevel = i->m_parentTransformComponent->m_transformHierarchyLevel + 1;
		}

	}
	//from top to bottom
	std::sort(g_GameSystemComponent->m_TransformComponents.begin(), g_GameSystemComponent->m_TransformComponents.end(), [&](TransformComponent* a, TransformComponent* b)
	{
		return a->m_transformHierarchyLevel < b->m_transformHierarchyLevel;
	});
}

void InnoGameSystemNS::updateTransform()
{
	// @TODO: update from hierarchy's top to down
	std::for_each(InnoGameSystemNS::g_GameSystemComponent->m_TransformComponents.begin(), InnoGameSystemNS::g_GameSystemComponent->m_TransformComponents.end(), [&](TransformComponent* val)
	{
		val->m_localTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_localTransformVector);
		val->m_globalTransformVector = InnoMath::LocalTransformVectorToGlobal(val->m_localTransformVector, val->m_parentTransformComponent->m_globalTransformVector, val->m_parentTransformComponent->m_globalTransformMatrix);
		val->m_globalTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_globalTransformVector);
	});
}

// @TODO: add a cache function for after-rendering business
INNO_SYSTEM_EXPORT void InnoGameSystem::saveComponentsCapture()
{
	std::for_each(InnoGameSystemNS::g_GameSystemComponent->m_TransformComponents.begin(), InnoGameSystemNS::g_GameSystemComponent->m_TransformComponents.end(), [&](TransformComponent* val)
	{
		val->m_globalTransformMatrix_prev = val->m_globalTransformMatrix;
	});
}

INNO_SYSTEM_EXPORT void InnoGameSystem::setGameInstance(IGameInstance * rhs)
{
	InnoGameSystemNS::m_gameInstance = rhs;
}

INNO_SYSTEM_EXPORT bool InnoGameSystem::initialize()
{
	InnoGameSystemNS::sortTransformComponentsVector();
	InnoGameSystemNS::updateTransform();

	if (!InnoGameSystemNS::m_gameInstance->initialize())
	{
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoGameSystem::update()
{
	auto temp = g_pCoreSystem->getTaskSystem()->submit([]()
	{
		InnoGameSystemNS::updateTransform();
	});
	InnoGameSystemNS::g_GameSystemComponent->m_asyncTask = &temp;

	if (!GameSystemComponent::get().m_pauseGameUpdate)
	{
		if (!InnoGameSystemNS::m_gameInstance->update())
		{
			return false;
		}
	}

	return true;
}

INNO_SYSTEM_EXPORT bool InnoGameSystem::terminate()
{
	if (!InnoGameSystemNS::m_gameInstance->terminate())
	{
		return false;
	}

	InnoGameSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameSystem has been terminated.");
	return true;
}

#define spawnComponentImplDefi( className ) \
INNO_SYSTEM_EXPORT void InnoGameSystem::spawnComponent(className* rhs, EntityID parentEntity) \
{ \
	rhs->m_parentEntity = parentEntity; \
	InnoGameSystemNS::g_GameSystemComponent->m_##className##s.emplace_back(rhs); \
	InnoGameSystemNS::g_GameSystemComponent->m_##className##sMap.emplace(parentEntity, rhs); \
}

spawnComponentImplDefi(TransformComponent)
spawnComponentImplDefi(VisibleComponent)
spawnComponentImplDefi(DirectionalLightComponent)
spawnComponentImplDefi(PointLightComponent)
spawnComponentImplDefi(SphereLightComponent)
spawnComponentImplDefi(CameraComponent)
spawnComponentImplDefi(InputComponent)
spawnComponentImplDefi(EnvironmentCaptureComponent)

INNO_SYSTEM_EXPORT std::string InnoGameSystem::getGameName()
{
	return std::string("GameInstance");
}

#define getComponentImplDefi( className ) \
INNO_SYSTEM_EXPORT className* InnoGameSystem::get##className(EntityID parentEntity) \
{ \
	auto result = InnoGameSystemNS::g_GameSystemComponent->m_##className##sMap.find(parentEntity); \
	if (result != InnoGameSystemNS::g_GameSystemComponent->m_##className##sMap.end()) \
	{ \
		return result->second; \
	} \
	else \
	{ \
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem : can't find " + std::string(#className) + " by EntityID: " + std::to_string(parentEntity) + " !"); \
		return nullptr; \
	} \
}

getComponentImplDefi(TransformComponent)
getComponentImplDefi(VisibleComponent)
getComponentImplDefi(DirectionalLightComponent)
getComponentImplDefi(PointLightComponent)
getComponentImplDefi(SphereLightComponent)
getComponentImplDefi(CameraComponent)
getComponentImplDefi(InputComponent)
getComponentImplDefi(EnvironmentCaptureComponent)

INNO_SYSTEM_EXPORT void InnoGameSystem::registerButtonStatusCallback(InputComponent * inputComponent, ButtonData boundButton, std::function<void()>* function)
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

INNO_SYSTEM_EXPORT TransformComponent * InnoGameSystem::getRootTransformComponent()
{
	return InnoGameSystemNS::g_GameSystemComponent->m_rootTransformComponent;
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

INNO_SYSTEM_EXPORT ObjectStatus InnoGameSystem::getStatus()
{
	return InnoGameSystemNS::m_objectStatus;
}
