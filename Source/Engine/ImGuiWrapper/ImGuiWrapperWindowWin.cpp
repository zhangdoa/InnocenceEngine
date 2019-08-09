#include "ImGuiWrapperWindowWin.h"
#include "../Component/WinWindowSystemComponent.h"

#include "../ThirdParty/ImGui/imgui_impl_win32.cpp"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiWrapperWindowWinNS
{
	WindowEventCallbackFunctor m_windowEventCallbackFunctor;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool ImGuiWrapperWindowWin::setup()
{
	ImGuiWrapperWindowWinNS::m_windowEventCallbackFunctor = [](void* hWnd, unsigned int msg, unsigned int wParam, int lParam) {ImGui_ImplWin32_WndProcHandler((HWND)hWnd, msg, wParam, lParam); };
	g_pModuleManager->getWindowSystem()->addEventCallback(&ImGuiWrapperWindowWinNS::m_windowEventCallbackFunctor);

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWindowWin setup finished.");

	return true;
}

bool ImGuiWrapperWindowWin::initialize()
{
	ImGui_ImplWin32_Init(WinWindowSystemComponent::get().m_hwnd);
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWindowWin has been initialized.");

	return true;
}

bool ImGuiWrapperWindowWin::newFrame()
{
	ImGui_ImplWin32_NewFrame();
	return true;
}

bool ImGuiWrapperWindowWin::terminate()
{
	ImGui_ImplWin32_Shutdown();
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWindowWin has been terminated.");

	return true;
}