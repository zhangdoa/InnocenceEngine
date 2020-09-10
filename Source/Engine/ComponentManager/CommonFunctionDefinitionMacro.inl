#define CleanComponentContainers( className ) \
for (auto i : m_Components) \
{ \
	if (i->m_ObjectLifespan == ObjectLifespan::Scene) \
	{ \
		i->m_ObjectStatus = ObjectStatus::Terminated; \
		m_ComponentPool->Destroy(i); \
	} \
} \
 \
m_Components.erase( \
	std::remove_if(m_Components.begin(), m_Components.end(), \
		[&](auto val) { \
	return val->m_ObjectLifespan == ObjectLifespan::Scene; \
}), m_Components.end()); \
 \
m_ComponentsMap.erase_if([&](auto val) { return val.second->m_ObjectLifespan == ObjectLifespan::Scene; });

#define SpawnComponentImpl( className ) \
	auto l_Component = m_ComponentPool->Spawn(); \
	if (l_Component) \
	{ \
		l_Component->m_UUID = InnoRandomizer::GenerateUUID(); \
		l_Component->m_ObjectStatus = ObjectStatus::Created; \
		l_Component->m_Serializable = serializable; \
		l_Component->m_ObjectLifespan = objectLifespan; \
		auto l_parentEntity = const_cast<InnoEntity*>(parentEntity); \
		l_Component->m_ParentEntity = l_parentEntity; \
		l_Component->m_ComponentType = g_pModuleManager->getFileSystem()->getComponentTypeID(#className); \
		auto l_componentIndex = m_CurrentComponentIndex; \
		auto l_componentName = ObjectName((std::string(parentEntity->m_Name.c_str()) + "." + std::string(#className) + "_" + std::to_string(l_componentIndex) + "/").c_str()); \
		l_Component->m_Name = l_componentName; \
		m_Components.emplace_back(l_Component); \
		m_ComponentsMap.emplace(l_parentEntity, l_Component); \
		l_Component->m_ObjectStatus = ObjectStatus::Activated; \
		m_CurrentComponentIndex++; \
\
		return l_Component; \
	} \
	else \
	{ \
		return nullptr; \
	}

#define DestroyComponentImpl( className ) \
	component->m_ObjectStatus = ObjectStatus::Terminated; \
	m_Components.eraseByValue(reinterpret_cast<className*>(component)); \
	m_ComponentsMap.erase(component->m_ParentEntity); \
	m_ComponentPool->Destroy(reinterpret_cast<className*>(component));

#define GetComponentImpl( className, parentEntity ) \
	auto l_parentEntity = const_cast<InnoEntity*>(parentEntity); \
	auto l_result = m_ComponentsMap.find(l_parentEntity); \
	if (l_result != m_ComponentsMap.end()) \
	{ \
		return l_result->second; \
	} \
	else \
	{ \
		InnoLogger::Log(LogLevel::Error, #className, "Manager: Can't find ", #className," by Entity: " ,l_parentEntity->m_Name.c_str(), "!"); \
		return nullptr; \
	}