#include "ImGuiWindowWin.h"
#include "ImGuiWindowWin.h"
#include "../../Platform/WinWindow/WinWindowSystem.h"

#include "../ImGui/imgui_impl_win32.cpp"

#include "../../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

namespace ImGuiWindowWinNS
{
	WindowEventCallback m_windowEventCallbackFunctor;

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

using namespace ImGuiWindowWinNS;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool ImGuiWindowWin::Setup(ISystemConfig* systemConfig)
{
	ImGuiWindowWinNS::m_windowEventCallbackFunctor = [](void* hWnd, uint32_t msg, uint64_t wParam, int64_t lParam) {
		ImGui_ImplWin32_WndProcHandler((HWND)hWnd, msg, wParam, lParam);
	};
	g_Engine->getWindowSystem()->AddEventCallback(&ImGuiWindowWinNS::m_windowEventCallbackFunctor);

	m_ObjectStatus = ObjectStatus::Created;

	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowWin Setup finished.");

	return true;
}

bool ImGuiWindowWin::Initialize()
{
	ImGui_ImplWin32_Init(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetWindowHandle());

	m_ObjectStatus = ObjectStatus::Activated;

	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowWin has been initialized.");

	return true;
}

bool ImGuiWindowWin::NewFrame()
{
	ImGui_ImplWin32_NewFrame();
	return true;
}

bool ImGuiWindowWin::Terminate()
{
	ImGui_ImplWin32_Shutdown();

	m_ObjectStatus = ObjectStatus::Terminated;

	g_Engine->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowWin has been terminated.");

	return true;
}

ObjectStatus ImGuiWindowWin::GetStatus()
{
	return m_ObjectStatus;
}