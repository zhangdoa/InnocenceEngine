#include "GameSystem.h"
#include "../common/config.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoGameSystemNS
{
	bool setup();

	std::string getEntityName(const EntityID& entityID);
	EntityID getEntityID(const std::string& entityName);

	void sortTransformComponentsVector();

	void updateTransformComponent();

	EntityID createEntity(const std::string & entityName);

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	// root TransformComponent
	TransformComponent* m_rootTransformComponent;

	void* m_TransformComponentPool;
	void* m_VisibleComponentPool;
	void* m_DirectionalLightComponentPool;
	void* m_PointLightComponentPool;
	void* m_SphereLightComponentPool;
	void* m_CameraComponentPool;
	void* m_InputComponentPool;
	void* m_EnvironmentCaptureComponentPool;

	// the AOS here
	ThreadSafeVector<TransformComponent*> m_TransformComponents;
	ThreadSafeVector<VisibleComponent*> m_VisibleComponents;
	ThreadSafeVector<DirectionalLightComponent*> m_DirectionalLightComponents;
	ThreadSafeVector<PointLightComponent*> m_PointLightComponents;
	ThreadSafeVector<SphereLightComponent*> m_SphereLightComponents;
	ThreadSafeVector<CameraComponent*> m_CameraComponents;
	ThreadSafeVector<InputComponent*> m_InputComponents;
	ThreadSafeVector<EnvironmentCaptureComponent*> m_EnvironmentCaptureComponents;

	ThreadSafeUnorderedMap<EntityID, TransformComponent*> m_TransformComponentsMap;
	ThreadSafeUnorderedMap<EntityID, VisibleComponent*> m_VisibleComponentsMap;
	ThreadSafeUnorderedMap<EntityID, DirectionalLightComponent*> m_DirectionalLightComponentsMap;
	ThreadSafeUnorderedMap<EntityID, PointLightComponent*> m_PointLightComponentsMap;
	ThreadSafeUnorderedMap<EntityID, SphereLightComponent*> m_SphereLightComponentsMap;
	ThreadSafeUnorderedMap<EntityID, CameraComponent*> m_CameraComponentsMap;
	ThreadSafeUnorderedMap<EntityID, InputComponent*> m_InputComponentsMap;
	ThreadSafeUnorderedMap<EntityID, EnvironmentCaptureComponent*> m_EnvironmentCaptureComponentsMap;

	EntityChildrenComponentsMetadataMap m_entityChildrenComponentsMetadataMap;
	EntityNameMap m_entityNameMap;

	unsigned int m_currentUUID = 0;

	InnoFuture<void>* m_asyncTask;

	std::function<void()> f_sceneLoadingStartCallback;
}

std::string InnoGameSystemNS::getEntityName(const EntityID& entityID)
{
	auto result = std::find_if(
		m_entityNameMap.begin(),
		m_entityNameMap.end(),
		[&](auto& val)-> bool {
		return val.first == entityID;
	}
	);

	if (result == m_entityNameMap.end())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: can't find entity name by ID " + entityID + " !");
		return "AbnormalEntityName";
	}

	return result->second;
}

EntityID InnoGameSystemNS::getEntityID(const std::string& entityName)
{
	auto result = std::find_if(
		m_entityNameMap.begin(),
		m_entityNameMap.end(),
		[&](auto& val)-> bool {
		return val.second == entityName;
	}
	);

	if (result == m_entityNameMap.end())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: can't find entity ID by name " + entityName + " !");
		return "AbnormalEntityID";
	}

	return result->first;
}

bool InnoGameSystemNS::setup()
{
	// allocate memory pool
	m_TransformComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(TransformComponent), 32768);
	m_VisibleComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(VisibleComponent), 16384);
	m_DirectionalLightComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(DirectionalLightComponent), 16);
	m_PointLightComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(PointLightComponent), 128);
	m_SphereLightComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(SphereLightComponent), 64);
	m_CameraComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(CameraComponent), 64);
	m_InputComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(InputComponent), 256);
	m_EnvironmentCaptureComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(EnvironmentCaptureComponent), 8192);

	// setup root TransformComponent
	m_rootTransformComponent = new TransformComponent();
	m_rootTransformComponent->m_parentTransformComponent = nullptr;

	m_rootTransformComponent->m_parentEntity = createEntity("RootTransform");

	m_rootTransformComponent->m_localTransformMatrix = InnoMath::TransformVectorToTransformMatrix(m_rootTransformComponent->m_localTransformVector);
	m_rootTransformComponent->m_globalTransformVector = m_rootTransformComponent->m_localTransformVector;
	m_rootTransformComponent->m_globalTransformMatrix = m_rootTransformComponent->m_localTransformMatrix;

	m_objectStatus = ObjectStatus::ALIVE;

	return true;
}

bool InnoGameSystem::setup()
{
	if (!InnoGameSystemNS::setup())
	{
		return false;
	}

	InnoGameSystemNS::f_sceneLoadingStartCallback = [&]() {
		for (auto i : InnoGameSystemNS::m_TransformComponents)
		{
			destroy(i);
		}
		InnoGameSystemNS::m_TransformComponents.clear();
		InnoGameSystemNS::m_TransformComponentsMap.clear();

		for (auto i : InnoGameSystemNS::m_VisibleComponents)
		{
			destroy(i);
		}
		InnoGameSystemNS::m_VisibleComponents.clear();
		InnoGameSystemNS::m_VisibleComponentsMap.clear();

		for (auto i : InnoGameSystemNS::m_DirectionalLightComponents)
		{
			destroy(i);
		}
		InnoGameSystemNS::m_DirectionalLightComponents.clear();
		InnoGameSystemNS::m_DirectionalLightComponentsMap.clear();

		for (auto i : InnoGameSystemNS::m_PointLightComponents)
		{
			destroy(i);
		}
		InnoGameSystemNS::m_PointLightComponents.clear();
		InnoGameSystemNS::m_PointLightComponentsMap.clear();

		for (auto i : InnoGameSystemNS::m_SphereLightComponents)
		{
			destroy(i);
		}
		InnoGameSystemNS::m_SphereLightComponents.clear();
		InnoGameSystemNS::m_SphereLightComponentsMap.clear();

		for (auto i : InnoGameSystemNS::m_CameraComponents)
		{
			destroy(i);
		}
		InnoGameSystemNS::m_CameraComponents.clear();
		InnoGameSystemNS::m_CameraComponentsMap.clear();

		for (auto i : InnoGameSystemNS::m_InputComponents)
		{
			destroy(i);
		}
		InnoGameSystemNS::m_InputComponents.clear();
		InnoGameSystemNS::m_InputComponentsMap.clear();

		for (auto i : InnoGameSystemNS::m_EnvironmentCaptureComponents)
		{
			destroy(i);
		}
		InnoGameSystemNS::m_EnvironmentCaptureComponents.clear();
		InnoGameSystemNS::m_EnvironmentCaptureComponentsMap.clear();
	};

	g_pCoreSystem->getFileSystem()->addSceneLoadingStartCallback(&InnoGameSystemNS::f_sceneLoadingStartCallback);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameInstance setup finished.");

	return true;
}

void InnoGameSystemNS::sortTransformComponentsVector()
{
	//construct the hierarchy tree
	for (auto i : m_TransformComponents)
	{
		if (i->m_parentTransformComponent)
		{
			i->m_transformHierarchyLevel = i->m_parentTransformComponent->m_transformHierarchyLevel + 1;
		}
	}
	//from top to bottom
	std::sort(m_TransformComponents.begin(), m_TransformComponents.end(), [&](TransformComponent* a, TransformComponent* b)
	{
		return a->m_transformHierarchyLevel < b->m_transformHierarchyLevel;
	});
}

void InnoGameSystemNS::updateTransformComponent()
{
	std::for_each(m_TransformComponents.begin(), m_TransformComponents.end(), [&](TransformComponent* val)
	{
		if (val->m_parentTransformComponent)
		{
			val->m_localTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_localTransformVector);
			val->m_globalTransformVector = InnoMath::LocalTransformVectorToGlobal(val->m_localTransformVector, val->m_parentTransformComponent->m_globalTransformVector, val->m_parentTransformComponent->m_globalTransformMatrix);
			val->m_globalTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_globalTransformVector);
		}
	});
}

// @TODO: add a cache function for after-rendering business
void InnoGameSystem::saveComponentsCapture()
{
	std::for_each(InnoGameSystemNS::m_TransformComponents.begin(), InnoGameSystemNS::m_TransformComponents.end(), [&](TransformComponent* val)
	{
		val->m_globalTransformMatrix_prev = val->m_globalTransformMatrix;
	});
}

EntityID InnoGameSystemNS::createEntity(const std::string & entityName)
{
	auto result = std::find_if(
		m_entityNameMap.begin(),
		m_entityNameMap.end(),
		[&](auto& val) -> bool {
		return val.second == entityName;
	});

	if (result != m_entityNameMap.end())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: duplicated entity name " + entityName + "!");
		return 0;
	}

	auto l_entityID = InnoMath::createEntityID();
	m_entityNameMap.emplace(l_entityID, entityName);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GameSystem: entity " + entityName + " has been created.");

	return l_entityID;
}

EntityID InnoGameSystem::createEntity(const std::string & entityName)
{
	return InnoGameSystemNS::createEntity(entityName);
}

bool InnoGameSystem::removeEntity(const std::string & entityName)
{
	for (auto i : InnoGameSystemNS::m_entityNameMap)
	{
		if (i.second == entityName)
		{
			InnoGameSystemNS::m_entityNameMap.erase(i.first);
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GameSystem: entity " + entityName + " has been removed.");
			return true;
		}
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "GameSystem: can't remove entity " + entityName + "!");
	return false;
}

std::string InnoGameSystem::getEntityName(const EntityID & entityID)
{
	return InnoGameSystemNS::getEntityName(entityID);
}

EntityID InnoGameSystem::getEntityID(const std::string & entityName)
{
	return InnoGameSystemNS::getEntityID(entityName);
}

bool InnoGameSystem::initialize()
{
	InnoGameSystemNS::sortTransformComponentsVector();
	InnoGameSystemNS::updateTransformComponent();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameSystem has been initialized.");
	return true;
}

bool InnoGameSystem::update()
{
	auto temp = g_pCoreSystem->getTaskSystem()->submit([&]()
	{
		InnoGameSystemNS::updateTransformComponent();
	});
	InnoGameSystemNS::m_asyncTask = &temp;

	return true;
}

bool InnoGameSystem::terminate()
{
	delete InnoGameSystemNS::m_rootTransformComponent;

	InnoGameSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameSystem has been terminated.");
	return true;
}

#define spawnComponentImplDefi( className ) \
className* InnoGameSystem::spawn##className(const EntityID& parentEntity) \
{ \
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(InnoGameSystemNS::m_##className##Pool, sizeof(className)); \
	auto l_ptr = new(l_rawPtr)className(); \
	if (l_ptr) \
	{ \
		registerComponent(l_ptr, parentEntity); \
		return l_ptr; \
	} \
	else \
	{ \
		return nullptr; \
	} \
}

spawnComponentImplDefi(TransformComponent)
spawnComponentImplDefi(VisibleComponent)
spawnComponentImplDefi(DirectionalLightComponent)
spawnComponentImplDefi(PointLightComponent)
spawnComponentImplDefi(SphereLightComponent)
spawnComponentImplDefi(CameraComponent)
spawnComponentImplDefi(InputComponent)
spawnComponentImplDefi(EnvironmentCaptureComponent)

#define destroyComponentImplDefi( className ) \
bool InnoGameSystem::destroy(className* rhs) \
{ \
	unregisterComponent(rhs); \
	return g_pCoreSystem->getMemorySystem()->destroyObject(InnoGameSystemNS::m_##className##Pool, sizeof(className), (void*)rhs); \
}

destroyComponentImplDefi(TransformComponent)
destroyComponentImplDefi(VisibleComponent)
destroyComponentImplDefi(DirectionalLightComponent)
destroyComponentImplDefi(PointLightComponent)
destroyComponentImplDefi(SphereLightComponent)
destroyComponentImplDefi(CameraComponent)
destroyComponentImplDefi(InputComponent)
destroyComponentImplDefi(EnvironmentCaptureComponent)

#define registerComponentImplDefi( className ) \
void InnoGameSystem::registerComponent(className* rhs, const EntityID& parentEntity) \
{ \
	rhs->m_parentEntity = parentEntity; \
	rhs->m_UUID = InnoGameSystemNS::m_currentUUID++; \
	InnoGameSystemNS::m_##className##s.emplace_back(rhs); \
	InnoGameSystemNS::m_##className##sMap.emplace(parentEntity, rhs); \
\
	auto indexOfTheComponent = InnoGameSystemNS::m_##className##s.size(); \
	auto l_componentName = std::string(#className) + "_" + std::to_string(indexOfTheComponent); \
	auto l_componentType = InnoUtility::getComponentType<className>(); \
	auto l_componentMetaDataPair = ComponentMetadataPair(l_componentType, l_componentName); \
\
	auto result = InnoGameSystemNS::m_entityChildrenComponentsMetadataMap.find(parentEntity); \
	if (result != InnoGameSystemNS::m_entityChildrenComponentsMetadataMap.end()) \
	{ \
		auto l_componentMetadataMap = &result->second; \
		l_componentMetadataMap->emplace(rhs, l_componentMetaDataPair); \
	} \
	else \
	{ \
		auto l_componentMetadataMap = ComponentMetadataMap(); \
		l_componentMetadataMap.emplace(rhs, l_componentMetaDataPair); \
		InnoGameSystemNS::m_entityChildrenComponentsMetadataMap.emplace(parentEntity, std::move(l_componentMetadataMap)); \
	} \
}

registerComponentImplDefi(TransformComponent)
registerComponentImplDefi(VisibleComponent)
registerComponentImplDefi(DirectionalLightComponent)
registerComponentImplDefi(PointLightComponent)
registerComponentImplDefi(SphereLightComponent)
registerComponentImplDefi(CameraComponent)
registerComponentImplDefi(InputComponent)
registerComponentImplDefi(EnvironmentCaptureComponent)

#define  unregisterComponentImplDefi( className ) \
void InnoGameSystem::unregisterComponent(className* rhs) \
{ \
	auto result = InnoGameSystemNS::m_entityChildrenComponentsMetadataMap.find(rhs->m_parentEntity); \
	if (result != InnoGameSystemNS::m_entityChildrenComponentsMetadataMap.end()) \
	{ \
		auto l_componentMetadataMap = &result->second; \
		l_componentMetadataMap->erase(rhs); \
	} \
	else \
	{ \
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: can't unregister " + std::string(#className) + " by EntityID: " + rhs->m_parentEntity + " !"); \
	} \
}

unregisterComponentImplDefi(TransformComponent)
unregisterComponentImplDefi(VisibleComponent)
unregisterComponentImplDefi(DirectionalLightComponent)
unregisterComponentImplDefi(PointLightComponent)
unregisterComponentImplDefi(SphereLightComponent)
unregisterComponentImplDefi(CameraComponent)
unregisterComponentImplDefi(InputComponent)
unregisterComponentImplDefi(EnvironmentCaptureComponent)

#define getComponentImplDefi( className ) \
className* InnoGameSystem::get##className(const EntityID& parentEntity) \
{ \
	auto result = InnoGameSystemNS::m_##className##sMap.find(parentEntity); \
	if (result != InnoGameSystemNS::m_##className##sMap.end()) \
	{ \
		return result->second; \
	} \
	else \
	{ \
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: can't find " + std::string(#className) + " by EntityID: " + parentEntity + " !"); \
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

#define getComponentContainerImplDefi( className ) \
std::vector<className*>& InnoGameSystem::get##className##s() \
{ \
	return InnoGameSystemNS::m_##className##s.getRawData(); \
}

getComponentContainerImplDefi(TransformComponent)
getComponentContainerImplDefi(VisibleComponent)
getComponentContainerImplDefi(DirectionalLightComponent)
getComponentContainerImplDefi(PointLightComponent)
getComponentContainerImplDefi(SphereLightComponent)
getComponentContainerImplDefi(CameraComponent)
getComponentContainerImplDefi(InputComponent)
getComponentContainerImplDefi(EnvironmentCaptureComponent)

std::string InnoGameSystem::getGameName()
{
	return std::string("GameInstance");
}

void InnoGameSystem::registerButtonStatusCallback(InputComponent * inputComponent, ButtonData boundButton, std::function<void()>* function)
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
	g_pCoreSystem->getInputSystem()->addButtonStatusCallback(boundButton, function);
}

TransformComponent* InnoGameSystem::getRootTransformComponent()
{
	return InnoGameSystemNS::m_rootTransformComponent;
}

EntityNameMap& InnoGameSystem::getEntityNameMap()
{
	return InnoGameSystemNS::m_entityNameMap;
}

EntityChildrenComponentsMetadataMap& InnoGameSystem::getEntityChildrenComponentsMetadataMap()
{
	return InnoGameSystemNS::m_entityChildrenComponentsMetadataMap;
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
	g_pCoreSystem->getInputSystem()->addMouseMovementCallback(mouseCode, function);
}

ObjectStatus InnoGameSystem::getStatus()
{
	return InnoGameSystemNS::m_objectStatus;
}