#include "VKRenderingSystem.h"

INNO_PRIVATE_SCOPE VKRenderingSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
}

INNO_SYSTEM_EXPORT bool VKRenderingSystem::setup()
{
	VKRenderingSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool VKRenderingSystem::initialize()
{
	return true;
}

INNO_SYSTEM_EXPORT bool VKRenderingSystem::update()
{
	return true;
}

INNO_SYSTEM_EXPORT bool VKRenderingSystem::terminate()
{
	VKRenderingSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	return true;
}

INNO_SYSTEM_EXPORT ObjectStatus VKRenderingSystem::getStatus()
{
	return VKRenderingSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT bool VKRenderingSystem::resize()
{
	return true;
}
