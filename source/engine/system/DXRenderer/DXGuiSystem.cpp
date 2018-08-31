#include "DXGuiSystem.h"

void DXGuiSystem::setup()
{
}

void DXGuiSystem::initialize()
{
	g_pLogSystem->printLog("DXGuiSystem has been initialized.");
}

void DXGuiSystem::update()
{
}

void DXGuiSystem::shutdown()
{
	m_objectStatus = objectStatus::STANDBY;

	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogSystem->printLog("DXGuiSystem has been shutdown.");
}

const objectStatus & DXGuiSystem::getStatus() const
{
	return m_objectStatus;
}