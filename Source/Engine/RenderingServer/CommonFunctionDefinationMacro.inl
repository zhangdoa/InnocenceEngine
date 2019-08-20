#define AddComponent(renderingServer, component) \
component##Component * renderingServer##RenderingServer::Add##component##Component(const char * name) \
{ \
static std::atomic<unsigned int> l_count = 0; \
l_count++; \
auto l_rawPtr = m_##component##ComponentPool->Spawn(); \
auto l_result = new(l_rawPtr)renderingServer##component##Component(); \
std::string l_name; \
if (strcmp(name, "")) \
{ \
	l_name = name; \
} \
else \
{ \
	l_name = (std::string(#component) + "_" + std::to_string(l_count) + "/"); \
} \
auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, l_name.c_str()); \
l_result->m_parentEntity = l_parentEntity; \
l_result->m_componentName = l_name.c_str(); \
l_result->m_objectSource = ObjectSource::Runtime; \
l_result->m_objectUsage = ObjectUsage::Engine; \
l_result->m_ComponentType = ComponentType::component##Component; \
 \
return l_result; \
}