#include "ImGuiWrapperWinDX.h"
#include "../Component/WinWindowSystemComponent.h"

#include "../ThirdParty/ImGui/imgui_impl_win32.h"
#include "../ThirdParty/ImGui/imgui_impl_dx11.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiWrapperWinDXNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
}

bool ImGuiWrapperWinDX11::setup()
{
	ImGuiWrapperWinDXNS::m_objectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWinDX setup finished.");

	return true;
}

bool ImGuiWrapperWinDX11::initialize()
{
	ImGui_ImplWin32_Init(WinWindowSystemComponent::get().m_hwnd);
	//ImGui_ImplDX11_Init(m_device, m_deviceContext);
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWinDX has been initialized.");

	return true;
}

bool ImGuiWrapperWinDX11::newFrame()
{
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX11_NewFrame();
	return true;
}

bool ImGuiWrapperWinDX11::render()
{
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return true;
}

bool ImGuiWrapperWinDX11::terminate()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGuiWrapperWinDXNS::m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWinDX has been terminated.");

	return true;
}

ObjectStatus ImGuiWrapperWinDX11::getStatus()
{
	return ImGuiWrapperWinDXNS::m_objectStatus;
}

void ImGuiWrapperWinDX11::showRenderResult(RenderPassType renderPassType)
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_renderTargetSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);
}

ImTextureID ImGuiWrapperWinDX11::getFileExplorerIconTextureID(const FileExplorerIconType iconType)
{
	return ImTextureID();
}