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
    return {};
}

IWindowSurface* HeadlessWindowService::GetWindowSurface()
{
    return m_dummySurface;
}

bool HeadlessWindowService::SendEvent(void* windowHook, uint32_t uMsg, uint32_t wParam, int32_t lParam)
{
    return true;
}

void HeadlessWindowService::ConsumeEvents(const WindowEventProcessCallback& p_Callback)
{
    std::vector<IWindowEvent*> emptyEvents;
    p_Callback(emptyEvents);
}

bool HeadlessWindowService::AddEventCallback(WindowEventCallback* callback)
{
    return true;
}
