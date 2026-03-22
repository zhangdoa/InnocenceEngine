#include "HeadlessWindowService.h"
#include "../../Interface/IWindowSurface.h"
#include "../../Common/LogService.h"
#include "../../Engine.h"

using namespace Inno;

bool HeadlessWindowService::Setup(IServiceConfig* systemConfig)
{
    Log(Success, "HeadlessWindowService: Setup complete (stub implementation).");
    m_ObjectStatus = ObjectStatus::Created;
    return true;
}

bool HeadlessWindowService::Initialize()
{
    Log(Success, "HeadlessWindowService: Initialize complete (stub implementation).");
    m_ObjectStatus = ObjectStatus::Activated;
    return true;
}

bool HeadlessWindowService::Update()
{
    // No window events to process in headless mode
    return true;
}

bool HeadlessWindowService::Terminate()
{
    Log(Success, "HeadlessWindowService: Terminate complete (stub implementation).");
    m_ObjectStatus = ObjectStatus::Terminated;
    return true;
}

ObjectStatus HeadlessWindowService::GetStatus()
{
    return m_ObjectStatus;
}

std::vector<std::type_index> HeadlessWindowService::GetDependencies()
{
    return {}; // No dependencies
}

IWindowSurface* HeadlessWindowService::GetWindowSurface()
{
    return m_dummySurface; // Returns null, headless mode has no surface
}

bool HeadlessWindowService::SendEvent(void* windowHook, uint32_t uMsg, uint32_t wParam, int32_t lParam)
{
    // No events to send in headless mode
    return true;
}

void HeadlessWindowService::ConsumeEvents(const WindowEventProcessCallback& p_Callback)
{
    // No events to consume in headless mode
    std::vector<IWindowEvent*> emptyEvents;
    p_Callback(emptyEvents);
}

bool HeadlessWindowService::AddEventCallback(WindowEventCallback* callback)
{
    // No events to callback in headless mode
    return true;
}
