#include "VKGuiSystem.h"

INNO_PRIVATE_SCOPE VKGuiSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
}

INNO_SYSTEM_EXPORT bool VKGuiSystem::setup()
{
	VKGuiSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool VKGuiSystem::initialize()
{
	return true;
}

INNO_SYSTEM_EXPORT bool VKGuiSystem::update()
{
	return true;
}

INNO_SYSTEM_EXPORT bool VKGuiSystem::terminate()
{
	VKGuiSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	return true;
}

INNO_SYSTEM_EXPORT ObjectStatus VKGuiSystem::getStatus()
{
	return VKGuiSystemNS::m_objectStatus;
}