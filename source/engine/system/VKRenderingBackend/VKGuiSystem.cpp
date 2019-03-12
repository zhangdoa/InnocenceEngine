#include "VKGuiSystem.h"

INNO_PRIVATE_SCOPE VKGuiSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
}

bool VKGuiSystem::setup()
{
	VKGuiSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

bool VKGuiSystem::initialize()
{
	return true;
}

bool VKGuiSystem::update()
{
	return true;
}

bool VKGuiSystem::terminate()
{
	VKGuiSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	return true;
}

ObjectStatus VKGuiSystem::getStatus()
{
	return VKGuiSystemNS::m_objectStatus;
}