#include "ImGuiWrapperDX11.h"
#include "../Component/WinWindowSystemComponent.h"

#include "../ThirdParty/ImGui/imgui_impl_dx11.cpp"

#include "../RenderingServer/DX11/DX11RenderingServer.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiWrapperDX11NS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
}

bool ImGuiWrapperDX11::setup()
{
	ImGuiWrapperDX11NS::m_objectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperDX11 setup finished.");

	return true;
}

bool ImGuiWrapperDX11::initialize()
{
	auto l_renderingServer = reinterpret_cast<DX11RenderingServer*>(g_pModuleManager->getRenderingServer());
	auto l_device = reinterpret_cast<ID3D11Device*>(l_renderingServer->GetDevice());
	auto l_deviceContext = reinterpret_cast<ID3D11DeviceContext*>(l_renderingServer->GetDeviceContext());

	ImGui_ImplDX11_Init(l_device, l_deviceContext);
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperDX11 has been initialized.");

	return true;
}

bool ImGuiWrapperDX11::newFrame()
{
	ImGui_ImplDX11_NewFrame();
	return true;
}

bool ImGuiWrapperDX11::render()
{
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return true;
}

bool ImGuiWrapperDX11::terminate()
{
	ImGui_ImplDX11_Shutdown();
	ImGuiWrapperDX11NS::m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperDX11 has been terminated.");

	return true;
}

ObjectStatus ImGuiWrapperDX11::getStatus()
{
	return ImGuiWrapperDX11NS::m_objectStatus;
}

void ImGuiWrapperDX11::showRenderResult(RenderPassType renderPassType)
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_renderTargetSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);
}

ImTextureID ImGuiWrapperDX11::getFileExplorerIconTextureID(const FileExplorerIconType iconType)
{
	return ImTextureID();
}