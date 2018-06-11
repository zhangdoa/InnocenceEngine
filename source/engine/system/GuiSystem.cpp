#include "GuiSystem.h"

void GuiSystem::setup()
{
	m_objectStatus = objectStatus::ALIVE;
}

void GuiSystem::initialize()
{
	g_pLogSystem->printLog("GuiSystem has been initialized.");

}

void GuiSystem::update()
{
}

void GuiSystem::shutdown()
{
	m_objectStatus = objectStatus::STANDBY;

	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogSystem->printLog("GuiSystem has been shutdown.");
}

const objectStatus & GuiSystem::getStatus() const
{
	return m_objectStatus;
}
