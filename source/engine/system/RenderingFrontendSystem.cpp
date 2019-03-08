#include "RenderingFrontendSystem.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoRenderingFrontendSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	ThreadSafeQueue<MeshDataComponent*> m_uninitializedMDC;
	ThreadSafeQueue<TextureDataComponent*> m_uninitializedTDC;
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::setup()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::initialize()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::update()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::terminate()
{
	return true;
}

INNO_SYSTEM_EXPORT ObjectStatus InnoRenderingFrontendSystem::getStatus()
{
	return InnoRenderingFrontendSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::anyUninitializedMeshDataComponent()
{
	return InnoRenderingFrontendSystemNS::m_uninitializedMDC.size() > 0;
}

INNO_SYSTEM_EXPORT bool InnoRenderingFrontendSystem::anyUninitializedTextureDataComponent()
{
	return InnoRenderingFrontendSystemNS::m_uninitializedTDC.size() > 0;
}

INNO_SYSTEM_EXPORT void InnoRenderingFrontendSystem::registerUninitializedMeshDataComponent(MeshDataComponent * rhs)
{
	InnoRenderingFrontendSystemNS::m_uninitializedMDC.push(rhs);
}

INNO_SYSTEM_EXPORT void InnoRenderingFrontendSystem::registerUninitializedTextureDataComponent(TextureDataComponent * rhs)
{
	InnoRenderingFrontendSystemNS::m_uninitializedTDC.push(rhs);
}

INNO_SYSTEM_EXPORT MeshDataComponent * InnoRenderingFrontendSystem::acquireUninitializedMeshDataComponent()
{
	MeshDataComponent* l_result;
	InnoRenderingFrontendSystemNS::m_uninitializedMDC.tryPop(l_result);
	return l_result;
}

INNO_SYSTEM_EXPORT TextureDataComponent * InnoRenderingFrontendSystem::acquireUninitializedTextureDataComponent()
{
	TextureDataComponent* l_result;
	InnoRenderingFrontendSystemNS::m_uninitializedTDC.tryPop(l_result);
	return l_result;
}
