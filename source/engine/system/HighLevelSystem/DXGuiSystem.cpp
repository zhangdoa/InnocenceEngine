#include "DXGuiSystem.h"
#include "../../component/LogSystemSingletonComponent.h"

namespace DXGuiSystem
{
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
}

void DXGuiSystem::Instance::setup()
{
}

void DXGuiSystem::Instance::initialize()
{
	LogSystemSingletonComponent::getInstance().m_log.push("DXGuiSystem has been initialized.");
}

void DXGuiSystem::Instance::update()
{
}

void DXGuiSystem::Instance::shutdown()
{
	m_objectStatus = objectStatus::STANDBY;

	m_objectStatus = objectStatus::SHUTDOWN;
	LogSystemSingletonComponent::getInstance().m_log.push("DXGuiSystem has been shutdown.");
}

objectStatus DXGuiSystem::Instance::getStatus()
{
	return m_objectStatus;
}