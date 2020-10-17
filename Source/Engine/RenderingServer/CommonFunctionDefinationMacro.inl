#define AddComponent(renderingServer, component) \
component##Component * renderingServer##RenderingServer::Add##component##Component(const char * name) \
{ \
static std::atomic<uint32_t> l_count = 0; \
l_count++; \
std::string l_name; \
if (strcmp(name, "")) \
{ \
	l_name = name; \
} \
else \
{ \
	l_name = (std::string(#component) + "_" + std::to_string(l_count) + "/"); \
} \
auto l_rawPtr = m_##component##ComponentPool->Spawn(); \
auto l_result = new(l_rawPtr)renderingServer##component##Component(); \
l_result->m_UUID = InnoRandomizer::GenerateUUID(); \
l_result->m_ObjectStatus = ObjectStatus::Created; \
l_result->m_Serializable = false; \
l_result->m_ObjectLifespan = ObjectLifespan::Persistence; \
auto l_parentEntity = g_Engine->getEntityManager()->Spawn(false, ObjectLifespan::Persistence, l_name.c_str()); \
l_result->m_Owner = l_parentEntity; \
l_result->m_InstanceName = l_name.c_str(); \
 \
return l_result; \
}