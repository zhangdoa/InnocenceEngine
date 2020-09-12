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
		auto l_owner = const_cast<InnoEntity*>(parentEntity); \
		l_Component->m_Owner = l_owner; \
		auto l_componentIndex = m_CurrentComponentIndex; \
		auto l_componentName = ObjectName((std::string(parentEntity->m_InstanceName.c_str()) + "." + std::string(l_Component->GetTypeName()) + "_" + std::to_string(l_componentIndex) + "/").c_str()); \
		l_Component->m_InstanceName = l_componentName; \
		m_Components.emplace_back(l_Component); \
		m_ComponentsMap.emplace(l_owner, l_Component); \
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
	m_ComponentsMap.erase(component->m_Owner); \
	m_ComponentPool->Destroy(reinterpret_cast<className*>(component));

#define GetComponentImpl( className, owner ) \
	auto l_owner = const_cast<InnoEntity*>(owner); \
	auto l_result = m_ComponentsMap.find(l_owner); \
	if (l_result != m_ComponentsMap.end()) \
	{ \
		return l_result->second; \
	} \
	else \
	{ \
		InnoLogger::Log(LogLevel::Error, #className, "Manager: Can't find ", #className," by Entity: " ,l_owner->m_InstanceName.c_str(), "!"); \
		return nullptr; \
	}