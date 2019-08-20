#define CleanComponentContainers( className ) \
for (auto i : m_Components) \
{ \
	if (i->m_objectUsage == ObjectUsage::Gameplay) \
	{ \
		i->m_objectStatus = ObjectStatus::Terminated; \
		m_ComponentPool->Destroy(i); \
	} \
} \
 \
m_Components.erase( \
	std::remove_if(m_Components.begin(), m_Components.end(), \
		[&](auto val) { \
	return val->m_objectUsage == ObjectUsage::Gameplay; \
}), m_Components.end()); \
 \
m_ComponentsMap.erase_if([&](auto val) { return val.second->m_objectUsage == ObjectUsage::Gameplay; });

#define SpawnComponentImpl( className ) \
	auto l_rawPtr= m_ComponentPool->Spawn(); \
	auto l_Component = new(l_rawPtr)className(); \
	if (l_Component) \
	{ \
		auto l_parentEntity = const_cast<InnoEntity*>(parentEntity); \
		l_Component->m_parentEntity = l_parentEntity; \
		l_Component->m_ComponentType = ComponentType::className; \
		l_Component->m_objectStatus = ObjectStatus::Created; \
		l_Component->m_objectSource = objectSource; \
		l_Component->m_objectUsage = objectUsage; \
		auto l_componentIndex = m_CurrentComponentIndex; \
		auto l_componentName = ComponentName((std::string(parentEntity->m_entityName.c_str()) + "." + std::string(#className) + "_" + std::to_string(l_componentIndex) + "/").c_str()); \
		l_Component->m_componentName = l_componentName; \
		l_Component->m_UUID = g_pModuleManager->getEntityManager()->AcquireUUID(); \
		m_Components.emplace_back(l_Component); \
		m_ComponentsMap.emplace(l_parentEntity, l_Component); \
		l_Component->m_objectStatus = ObjectStatus::Activated; \
		m_CurrentComponentIndex++; \
\
		return l_Component; \
	} \
	else \
	{ \
		return nullptr; \
	}

#define DestroyComponentImpl( className ) \
	component->m_objectStatus = ObjectStatus::Terminated; \
	m_Components.eraseByValue(reinterpret_cast<className*>(component)); \
	m_ComponentsMap.erase(component->m_parentEntity); \
	m_ComponentPool->Destroy(component);

#define GetComponentImpl( className, parentEntity ) \
	auto l_parentEntity = const_cast<InnoEntity*>(parentEntity); \
	auto l_result = m_ComponentsMap.find(l_parentEntity); \
	if (l_result != m_ComponentsMap.end()) \
	{ \
		return l_result->second; \
	} \
	else \
	{ \
		InnoLogger::Log(LogLevel::Error, #className, "Manager: Can't find ", #className," by Entity: " ,l_parentEntity->m_entityName.c_str(), "!"); \
		return nullptr; \
	}