#include "DXGuiSystem.h"
#include "../LogSystem.h"

namespace DXGuiSystem
{
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
}

void DXGuiSystem::setup()
{
}

void DXGuiSystem::initialize()
{
	InnoLogSystem::printLog("DXGuiSystem has been initialized.");
}

void DXGuiSystem::update()
{
}

void DXGuiSystem::shutdown()
{
	m_objectStatus = objectStatus::STANDBY;

	m_objectStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("DXGuiSystem has been shutdown.");
}

objectStatus DXGuiSystem::getStatus()
{
	return m_objectStatus;
}