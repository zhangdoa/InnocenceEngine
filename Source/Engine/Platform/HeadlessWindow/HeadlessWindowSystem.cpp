#include "HeadlessWindowSystem.h"
#include "../../Interface/IWindowSurface.h"
#include "../../Common/LogService.h"
#include "../../Engine.h"

using namespace Inno;

bool HeadlessWindowSystem::Setup(ISystemConfig* systemConfig)
{
    Log(Success, "HeadlessWindowSystem: Setup complete (stub implementation).");
    m_ObjectStatus = ObjectStatus::Created;
    return true;
}

bool HeadlessWindowSystem::Initialize()
{
    Log(Success, "HeadlessWindowSystem: Initialize complete (stub implementation).");
    m_ObjectStatus = ObjectStatus::Activated;
    return true;
}

bool HeadlessWindowSystem::Update()
{
    // No window events to process in headless mode
    return true;
}

bool HeadlessWindowSystem::Terminate()
{
    Log(Success, "HeadlessWindowSystem: Terminate complete (stub implementation).");
    m_ObjectStatus = ObjectStatus::Terminated;
    return true;
}

ObjectStatus HeadlessWindowSystem::GetStatus()
{
    return m_ObjectStatus;
}

std::vector<std::type_index> HeadlessWindowSystem::GetDependencies()
{
    return {}; // No dependencies
}

IWindowSurface* HeadlessWindowSystem::GetWindowSurface()
{
    return m_dummySurface; // Returns null, headless mode has no surface
}

bool HeadlessWindowSystem::SendEvent(void* windowHook, uint32_t uMsg, uint32_t wParam, int32_t lParam)
{
    // No events to send in headless mode
    return true;
}

void HeadlessWindowSystem::ConsumeEvents(const WindowEventProcessCallback& p_Callback)
{
    // No events to consume in headless mode
    std::vector<IWindowEvent*> emptyEvents;
    p_Callback(emptyEvents);
}

bool HeadlessWindowSystem::AddEventCallback(WindowEventCallback* callback)
{
    // No events to callback in headless mode
    return true;
}
