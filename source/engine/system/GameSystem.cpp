#include "GameSystem.h"
#include "../common/config.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

#define cleanContainers( className ) \
for (auto i : InnoGameSystemNS::m_##className##s) \
{ \
	if (i->m_objectUsage == ObjectUsage::Gameplay) \
	{ \
		destroy(i); \
	} \
} \
 \
InnoGameSystemNS::m_##className##s.erase( \
	std::remove_if(InnoGameSystemNS::m_##className##s.begin(), InnoGameSystemNS::m_##className##s.end(), \
		[&](auto val) { \
	return val->m_objectUsage == ObjectUsage::Gameplay; \
}), InnoGameSystemNS::m_##className##s.end()); \
 \
InnoGameSystemNS::m_##className##sMap.erase_if([&](auto val) { return val.second->m_objectUsage == ObjectUsage::Gameplay; });

INNO_PRIVATE_SCOPE InnoGameSystemNS
{
	bool setup();

	void sortTransformComponentsVector();

	void updateTransformComponent();

	InnoEntity* createEntity(const EntityName& entityName, ObjectSource objectSource, ObjectUsage objectUsage);
	bool removeEntity(const InnoEntity* entity);
	InnoEntity* getEntity(const EntityName& entityName);

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	void* m_EntityPool;

	void* m_TransformComponentPool;
	void* m_VisibleComponentPool;
	void* m_DirectionalLightComponentPool;
	void* m_PointLightComponentPool;
	void* m_SphereLightComponentPool;
	void* m_CameraComponentPool;
	void* m_InputComponentPool;
	void* m_EnvironmentCaptureComponentPool;

	ThreadSafeVector<InnoEntity*> m_Entities;
	ThreadSafeVector<TransformComponent*> m_TransformComponents;
	ThreadSafeVector<VisibleComponent*> m_VisibleComponents;
	ThreadSafeVector<DirectionalLightComponent*> m_DirectionalLightComponents;
	ThreadSafeVector<PointLightComponent*> m_PointLightComponents;
	ThreadSafeVector<SphereLightComponent*> m_SphereLightComponents;
	ThreadSafeVector<CameraComponent*> m_CameraComponents;
	ThreadSafeVector<InputComponent*> m_InputComponents;
	ThreadSafeVector<EnvironmentCaptureComponent*> m_EnvironmentCaptureComponents;

	ThreadSafeUnorderedMap<InnoEntity*, TransformComponent*> m_TransformComponentsMap;
	ThreadSafeUnorderedMap<InnoEntity*, VisibleComponent*> m_VisibleComponentsMap;
	ThreadSafeUnorderedMap<InnoEntity*, DirectionalLightComponent*> m_DirectionalLightComponentsMap;
	ThreadSafeUnorderedMap<InnoEntity*, PointLightComponent*> m_PointLightComponentsMap;
	ThreadSafeUnorderedMap<InnoEntity*, SphereLightComponent*> m_SphereLightComponentsMap;
	ThreadSafeUnorderedMap<InnoEntity*, CameraComponent*> m_CameraComponentsMap;
	ThreadSafeUnorderedMap<InnoEntity*, InputComponent*> m_InputComponentsMap;
	ThreadSafeUnorderedMap<InnoEntity*, EnvironmentCaptureComponent*> m_EnvironmentCaptureComponentsMap;

	EntityChildrenComponentsMetadataMap m_entityChildrenComponentsMetadataMap;
	std::set<EntityName> m_entityNameSet;

	unsigned int m_currentUUID = 0;

	std::function<void()> f_sceneLoadingStartCallback;

	// root TransformComponent
	TransformComponent* m_rootTransformComponent;
}

bool InnoGameSystemNS::setup()
{
	// allocate memory pool
	m_EntityPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(InnoEntity), 65536);

	m_TransformComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(TransformComponent), 32768);
	m_VisibleComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(VisibleComponent), 16384);
	m_DirectionalLightComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(DirectionalLightComponent), 16);
	m_PointLightComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(PointLightComponent), 1024);
	m_SphereLightComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(SphereLightComponent), 128);
	m_CameraComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(CameraComponent), 64);
	m_InputComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(InputComponent), 256);
	m_EnvironmentCaptureComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(EnvironmentCaptureComponent), 8192);

	return true;
}

bool InnoGameSystem::setup()
{
	if (!InnoGameSystemNS::setup())
	{
		return false;
	}

	InnoGameSystemNS::f_sceneLoadingStartCallback = [&]() {
		for (auto i : InnoGameSystemNS::m_Entities)
		{
			if (i->m_objectUsage == ObjectUsage::Gameplay)
			{
				removeEntity(i);
			}
		}

		InnoGameSystemNS::m_Entities.erase(
			std::remove_if(InnoGameSystemNS::m_Entities.begin(), InnoGameSystemNS::m_Entities.end(),
				[&](auto val) {
			return val->m_objectUsage == ObjectUsage::Gameplay;
		}), InnoGameSystemNS::m_Entities.end());

		cleanContainers(TransformComponent);
		cleanContainers(VisibleComponent);
		cleanContainers(DirectionalLightComponent);
		cleanContainers(PointLightComponent);
		cleanContainers(SphereLightComponent);
		cleanContainers(CameraComponent);
		cleanContainers(InputComponent);
		cleanContainers(EnvironmentCaptureComponent);
	};

	g_pCoreSystem->getFileSystem()->addSceneLoadingStartCallback(&InnoGameSystemNS::f_sceneLoadingStartCallback);

	// setup root TransformComponent
	auto l_entity = createEntity("RootTransform/", ObjectSource::Runtime, ObjectUsage::Engine);

	InnoGameSystemNS::m_rootTransformComponent = spawn<TransformComponent>(l_entity, ObjectSource::Runtime, ObjectUsage::Engine);
	InnoGameSystemNS::m_rootTransformComponent->m_parentTransformComponent = nullptr;

	InnoGameSystemNS::m_rootTransformComponent->m_localTransformMatrix = InnoMath::TransformVectorToTransformMatrix(InnoGameSystemNS::m_rootTransformComponent->m_localTransformVector);
	InnoGameSystemNS::m_rootTransformComponent->m_globalTransformVector = InnoGameSystemNS::m_rootTransformComponent->m_localTransformVector;
	InnoGameSystemNS::m_rootTransformComponent->m_globalTransformMatrix = InnoGameSystemNS::m_rootTransformComponent->m_localTransformMatrix;

	// setup default CameraComponent
	InnoGameSystemNS::m_objectStatus = ObjectStatus::Created;

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

InnoEntity* InnoGameSystemNS::createEntity(const EntityName& entityName, ObjectSource objectSource, ObjectUsage objectUsage)
{
	auto l_result = std::find_if(
		m_entityNameSet.begin(),
		m_entityNameSet.end(),
		[&](auto val) -> bool {
		return val == entityName;
	});

	if (l_result != m_entityNameSet.end())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: duplicated entity name " + std::string(entityName.c_str()) + "!");
		return nullptr;
	}

	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(InnoGameSystemNS::m_EntityPool, sizeof(InnoEntity));
	auto l_ptr = new(l_rawPtr)InnoEntity();
	if (l_ptr)
	{
		auto l_entityID = InnoMath::createEntityID();
		m_entityNameSet.emplace(entityName);

		l_ptr->m_objectStatus = ObjectStatus::Created;
		m_Entities.emplace_back(l_ptr);

		l_ptr->m_entityID = l_entityID;
		l_ptr->m_entityName = entityName;
		l_ptr->m_objectSource = objectSource;
		l_ptr->m_objectUsage = objectUsage;
		l_ptr->m_objectStatus = ObjectStatus::Activated;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GameSystem: entity " + std::string(entityName.c_str()) + " has been created.");

	return l_ptr;
}

InnoEntity* InnoGameSystem::createEntity(const EntityName& entityName, ObjectSource objectSource, ObjectUsage objectUsage)
{
	return InnoGameSystemNS::createEntity(entityName, objectSource, objectUsage);
}

bool InnoGameSystemNS::removeEntity(const InnoEntity* entity)
{
	auto l_result = std::find_if(
		m_entityNameSet.begin(),
		m_entityNameSet.end(),
		[&](auto val) -> bool {
		return val == entity->m_entityName;
	});

	if (l_result != m_entityNameSet.end())
	{
		InnoGameSystemNS::m_entityNameSet.erase(l_result);
		g_pCoreSystem->getMemorySystem()->destroyObject(InnoGameSystemNS::m_EntityPool, sizeof(InnoEntity), (void*)entity);
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: can't find entity " + std::string(entity->m_entityName.c_str()) + " to remove.");
		return false;
	}
}

InnoEntity* InnoGameSystemNS::getEntity(const EntityName& entityName)
{
	auto l_result = std::find_if(
		m_Entities.begin(),
		m_Entities.end(),
		[&](auto val) -> bool {
		return val->m_entityName == entityName;
	});

	if (l_result != m_Entities.end())
	{
		return *l_result;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "GameSystem: can't find entity " + std::string(entityName.c_str()) + "!");
		return nullptr;
	}
}

bool InnoGameSystem::removeEntity(const InnoEntity * entity)
{
	return InnoGameSystemNS::removeEntity(entity);
}

InnoEntity * InnoGameSystem::getEntity(const EntityName & entityName)
{
	return InnoGameSystemNS::getEntity(entityName);
}

bool InnoGameSystem::initialize()
{
	if (InnoGameSystemNS::m_objectStatus == ObjectStatus::Created)
	{
		InnoGameSystemNS::sortTransformComponentsVector();
		InnoGameSystemNS::updateTransformComponent();

		InnoGameSystemNS::m_objectStatus = ObjectStatus::Activated;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameSystem has been initialized.");
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: Object is not created!");
		return false;
	}
}

bool InnoGameSystem::update()
{
	if (InnoGameSystemNS::m_objectStatus == ObjectStatus::Activated)
	{
		auto updateTask = g_pCoreSystem->getTaskSystem()->submit("TransformComponentsUpdateTask", [&]()
		{
			InnoGameSystemNS::updateTransformComponent();
		});
		return true;
	}
	else
	{
		InnoGameSystemNS::m_objectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoGameSystem::terminate()
{
	delete InnoGameSystemNS::m_rootTransformComponent;

	InnoGameSystemNS::m_objectStatus = ObjectStatus::Terminated;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameSystem has been terminated.");
	return true;
}

#define spawnComponentImplDefi( className ) \
className* InnoGameSystem::spawn##className(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage) \
{ \
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(InnoGameSystemNS::m_##className##Pool, sizeof(className)); \
	auto l_ptr = new(l_rawPtr)className(); \
	if (l_ptr) \
	{ \
		auto l_parentEntity = const_cast<InnoEntity*>(parentEntity); \
		l_ptr->m_parentEntity = l_parentEntity; \
		l_ptr->m_objectStatus = ObjectStatus::Created; \
		l_ptr->m_objectSource = objectSource; \
		l_ptr->m_objectUsage = objectUsage; \
		auto l_componentIndex = InnoGameSystemNS::m_##className##s.size(); \
		auto l_componentName = ComponentName((std::string(#className) + "_" + std::to_string(l_componentIndex) + "/").c_str()); \
		l_ptr->m_componentName = l_componentName; \
		l_ptr->m_UUID = InnoGameSystemNS::m_currentUUID++; \
		InnoGameSystemNS::m_##className##s.emplace_back(l_ptr); \
		InnoGameSystemNS::m_##className##sMap.emplace(l_parentEntity, l_ptr); \
		l_ptr->m_objectStatus = ObjectStatus::Activated; \
\
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
	rhs->m_objectStatus = ObjectStatus::Terminated; \
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
void InnoGameSystem::registerComponent(className* rhs, const InnoEntity* parentEntity) \
{ \
	if(rhs->m_objectSource == ObjectSource::Asset) \
	{ \
		auto l_parentEntity = const_cast<InnoEntity*>(parentEntity); \
		auto l_componentType = InnoUtility::getComponentType<className>(); \
		auto l_componentMetaDataPair = ComponentMetadataPair(l_componentType, rhs->m_componentName); \
		\
		auto l_result = InnoGameSystemNS::m_entityChildrenComponentsMetadataMap.find(l_parentEntity); \
		if (l_result != InnoGameSystemNS::m_entityChildrenComponentsMetadataMap.end()) \
		{ \
			auto l_componentMetadataMap = &l_result->second; \
			l_componentMetadataMap->emplace(rhs, l_componentMetaDataPair); \
		} \
		else \
		{ \
			auto l_componentMetadataMap = ComponentMetadataMap(); \
			l_componentMetadataMap.emplace(rhs, l_componentMetaDataPair); \
			InnoGameSystemNS::m_entityChildrenComponentsMetadataMap.emplace(l_parentEntity, std::move(l_componentMetadataMap)); \
		} \
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
	if(rhs->m_objectSource == ObjectSource::Asset) \
	{ \
		auto l_result = InnoGameSystemNS::m_entityChildrenComponentsMetadataMap.find(rhs->m_parentEntity); \
		if (l_result != InnoGameSystemNS::m_entityChildrenComponentsMetadataMap.end()) \
		{ \
			auto l_componentMetadataMap = &l_result->second; \
			l_componentMetadataMap->erase(rhs); \
		} \
		else \
		{ \
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: can't unregister " + std::string(#className) + " by Entity: " + std::string(rhs->m_parentEntity->m_entityName.c_str()) + " !"); \
		} \
	}\
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
className* InnoGameSystem::get##className(const InnoEntity* parentEntity) \
{ \
	auto l_parentEntity = const_cast<InnoEntity*>(parentEntity); \
	auto l_result = InnoGameSystemNS::m_##className##sMap.find(l_parentEntity); \
	if (l_result != InnoGameSystemNS::m_##className##sMap.end()) \
	{ \
		return l_result->second; \
	} \
	else \
	{ \
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: can't find " + std::string(#className) + " by Entity: " + std::string(l_parentEntity->m_entityName.c_str()) + "!"); \
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

const std::vector<InnoEntity*>& InnoGameSystem::getEntities()
{
	return InnoGameSystemNS::m_Entities.getRawData();
}

const EntityChildrenComponentsMetadataMap& InnoGameSystem::getEntityChildrenComponentsMetadataMap()
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