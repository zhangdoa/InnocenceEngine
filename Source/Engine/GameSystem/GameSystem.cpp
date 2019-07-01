#include "GameSystem.h"
#include "../Common/Config.h"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

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
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	void* m_CameraComponentPool;

	ThreadSafeVector<CameraComponent*> m_CameraComponents;

	ThreadSafeUnorderedMap<InnoEntity*, CameraComponent*> m_CameraComponentsMap;

	EntityChildrenComponentsMetadataMap m_entityChildrenComponentsMetadataMap;
	std::set<EntityName> m_entityNameSet;

	unsigned int m_currentUUID = 0;

	std::function<void()> f_sceneLoadingStartCallback;
}

bool InnoGameSystemNS::setup()
{
	// allocate memory pool
	m_CameraComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(CameraComponent), 64);

	return true;
}

bool InnoGameSystem::setup()
{
	if (!InnoGameSystemNS::setup())
	{
		return false;
	}

	InnoGameSystemNS::f_sceneLoadingStartCallback = [&]() {
		cleanContainers(CameraComponent);
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&InnoGameSystemNS::f_sceneLoadingStartCallback);

	InnoGameSystemNS::m_objectStatus = ObjectStatus::Created;

	return true;
}

bool InnoGameSystem::initialize()
{
	if (InnoGameSystemNS::m_objectStatus == ObjectStatus::Created)
	{
		InnoGameSystemNS::m_objectStatus = ObjectStatus::Activated;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameSystem has been initialized.");
		return true;
	}
	else
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: Object is not created!");
		return false;
	}
}

bool InnoGameSystem::update()
{
	if (InnoGameSystemNS::m_objectStatus == ObjectStatus::Activated)
	{
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
	InnoGameSystemNS::m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameSystem has been terminated.");
	return true;
}

#define spawnComponentImplDefi( className ) \
className* InnoGameSystem::spawn##className(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage) \
{ \
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(InnoGameSystemNS::m_##className##Pool, sizeof(className)); \
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

spawnComponentImplDefi(CameraComponent)

#define destroyComponentImplDefi( className ) \
bool InnoGameSystem::destroy(className* rhs) \
{ \
	rhs->m_objectStatus = ObjectStatus::Terminated; \
	unregisterComponent(rhs); \
	return g_pModuleManager->getMemorySystem()->destroyObject(InnoGameSystemNS::m_##className##Pool, sizeof(className), (void*)rhs); \
}

destroyComponentImplDefi(CameraComponent)

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

registerComponentImplDefi(CameraComponent)

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
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: can't unregister " + std::string(#className) + " by Entity: " + std::string(rhs->m_parentEntity->m_entityName.c_str()) + " !"); \
		} \
	}\
}

unregisterComponentImplDefi(CameraComponent)

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
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: can't find " + std::string(#className) + " by Entity: " + std::string(l_parentEntity->m_entityName.c_str()) + "!"); \
		return nullptr; \
	} \
}

getComponentImplDefi(CameraComponent)

#define getComponentContainerImplDefi( className ) \
std::vector<className*>& InnoGameSystem::get##className##s() \
{ \
	return InnoGameSystemNS::m_##className##s.getRawData(); \
}

getComponentContainerImplDefi(CameraComponent)

std::string InnoGameSystem::getGameName()
{
	return std::string("GameInstance");
}

const EntityChildrenComponentsMetadataMap& InnoGameSystem::getEntityChildrenComponentsMetadataMap()
{
	return InnoGameSystemNS::m_entityChildrenComponentsMetadataMap;
}

ObjectStatus InnoGameSystem::getStatus()
{
	return InnoGameSystemNS::m_objectStatus;
}