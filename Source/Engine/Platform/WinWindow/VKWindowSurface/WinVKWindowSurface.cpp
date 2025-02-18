#include "WinVKWindowSurface.h"
#include "../WinWindowSystem.h"
#include "../../../Common/LogService.h"
#include "../../../Common/TaskScheduler.h"
#include "../../../Services/RenderingConfigurationService.h"
#include "../../../Services/RenderingContextService.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "../../../RenderingServer/VK/VKRenderingServer.h"

#include "../../../Engine.h"

using namespace Inno;

bool WinVKWindowSurface::Setup(ISystemConfig* systemConfig)
{
	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool WinVKWindowSurface::Initialize()
{
	VkWin32SurfaceCreateInfoKHR l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	l_createInfo.pNext = NULL;
	l_createInfo.hinstance = reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetApplicationInstance();
	l_createInfo.hwnd = reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetWindowHandle();

	auto l_renderingServer = reinterpret_cast<VKRenderingServer*>(g_Engine->getRenderingServer());
	auto l_VkInstance = reinterpret_cast<VkInstance>(l_renderingServer->GetVkInstance());
	auto l_VkSurface = reinterpret_cast<VkSurfaceKHR*>(l_renderingServer->GetVkSurface());

	if (vkCreateWin32SurfaceKHR(l_VkInstance, &l_createInfo, NULL, l_VkSurface) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		Log(Error, "Failed to create window surface!");
		return false;
	}

	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "WinVKWindowSurface has been initialized.");
	return true;
}

bool WinVKWindowSurface::Update()
{
	return true;
}

bool WinVKWindowSurface::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "WinVKWindowSurface has been terminated.");

	return true;
}

ObjectStatus WinVKWindowSurface::GetStatus()
{
	return m_ObjectStatus;
}