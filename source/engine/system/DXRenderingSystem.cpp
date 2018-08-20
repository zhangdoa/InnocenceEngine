#include "DXRenderingSystem.h"

void DXRenderingSystem::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void DXRenderingSystem::initialize()
{
	g_pLogSystem->printLog("DXRenderingSystem has been initialized.");
}

void DXRenderingSystem::update()
{
}

void DXRenderingSystem::shutdown()
{
	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogSystem->printLog("DXRenderingSystem has been shutdown.");
}

const objectStatus & DXRenderingSystem::getStatus() const
{
	return m_objectStatus;
}
