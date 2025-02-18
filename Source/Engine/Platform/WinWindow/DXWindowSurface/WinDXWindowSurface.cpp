#include "WinDXWindowSurface.h"
#include "../WinWindowSystem.h"
#include "../../../Common/LogService.h"
#include "../../../Services/RenderingConfigurationService.h"
#include "../../../Services/RenderingContextService.h"

#include "../../../Engine.h"
using namespace Inno;

bool WinDXWindowSurface::Setup(ISystemConfig* systemConfig)
{
	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool WinDXWindowSurface::Initialize()
{
	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool WinDXWindowSurface::Update()
{
	return true;
}

bool WinDXWindowSurface::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus WinDXWindowSurface::GetStatus()
{
	return m_ObjectStatus;
}