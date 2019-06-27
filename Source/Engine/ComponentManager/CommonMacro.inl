#define CleanComponentContainers( className ) \
for (auto i : m_Components) \
{ \
	if (i->m_objectUsage == ObjectUsage::Gameplay) \
	{ \
		Destory(i); \
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

#define SpawnComponent( className ) \
	auto l_Component = reinterpret_cast<className*>(m_ComponentPool->Spawn()); \
	if (l_Component) \
	{ \
		auto l_parentEntity = const_cast<InnoEntity*>(parentEntity); \
		l_Component->m_parentEntity = l_parentEntity; \
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

#define DestroyComponent( className ) \
	component->m_objectStatus = ObjectStatus::Terminated; \
	m_ComponentPool->Destroy(component);