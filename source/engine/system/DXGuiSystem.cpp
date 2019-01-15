#include "DXGuiSystem.h"
#include "../component/WindowSystemComponent.h"
#include "../component/DXWindowSystemComponent.h"
#include "../component/DXRenderingSystemComponent.h"

#include "ImGuiWrapper.h"
#include "../third-party/ImGui/imgui_impl_win32.h"
#include "../third-party/ImGui/imgui_impl_dx11.h"
#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE DXGuiSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	void showRenderResult(RenderingConfig & renderingConfig);
	ImTextureID getFileExplorerIconTextureID(const FileExplorerIconType iconType);

	std::function<void(RenderingConfig&)> f_ShowRenderPassResult;
	std::function<ImTextureID(const FileExplorerIconType)> f_GetFileExplorerIconTextureID;
}

INNO_SYSTEM_EXPORT bool DXGuiSystem::setup()
{
	DXGuiSystemNS::f_ShowRenderPassResult = DXGuiSystemNS::showRenderResult;
	DXGuiSystemNS::f_GetFileExplorerIconTextureID = DXGuiSystemNS::getFileExplorerIconTextureID;

	ImGuiWrapper::get().setup();

	ImGuiWrapper::get().addShowRenderPassResultCallback(&DXGuiSystemNS::f_ShowRenderPassResult);
	ImGuiWrapper::get().addGetFileExplorerIconTextureIDCallback(&DXGuiSystemNS::f_GetFileExplorerIconTextureID);

	DXGuiSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool DXGuiSystem::initialize()
{
	ImGui_ImplWin32_Init(DXWindowSystemComponent::get().m_hwnd);
	ImGui_ImplDX11_Init(DXRenderingSystemComponent::get().m_device, DXRenderingSystemComponent::get().m_deviceContext);

	ImGuiWrapper::get().initialize();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXGuiSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool DXGuiSystem::update()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGuiWrapper::get().update();

	// Rendering
	DXRenderingSystemComponent::get().m_deviceContext->OMSetRenderTargets(
		1,
		&DXRenderingSystemComponent::get().m_renderTargetView,
		NULL);

	DXRenderingSystemComponent::get().m_deviceContext->RSSetViewports(
		1,
		&DXRenderingSystemComponent::get().m_viewport);

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return true;
}

INNO_SYSTEM_EXPORT bool DXGuiSystem::terminate()
{
	DXGuiSystemNS::m_objectStatus = ObjectStatus::STANDBY;

#ifndef INNO_PLATFORM_MACOS
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();

	ImGuiWrapper::get().terminate();
#endif // !INNO_PLATFORM_MACOS

	DXGuiSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXGuiSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT ObjectStatus DXGuiSystem::getStatus()
{
	return DXGuiSystemNS::m_objectStatus;
}

void DXGuiSystemNS::showRenderResult(RenderingConfig & renderingConfig)
{
}

ImTextureID DXGuiSystemNS::getFileExplorerIconTextureID(const FileExplorerIconType iconType)
{
	switch (iconType)
	{
	case FileExplorerIconType::OBJ:
		return ImTextureID(DXRenderingSystemComponent::get().m_iconTemplate_OBJ->m_SRV); break;
	case FileExplorerIconType::PNG:
		return ImTextureID(DXRenderingSystemComponent::get().m_iconTemplate_PNG->m_SRV); break;
	case FileExplorerIconType::SHADER:
		return ImTextureID(DXRenderingSystemComponent::get().m_iconTemplate_SHADER->m_SRV); break;
	case FileExplorerIconType::UNKNOWN:
		return ImTextureID(DXRenderingSystemComponent::get().m_iconTemplate_UNKNOWN->m_SRV); break;
	default:
		return nullptr; break;
	}
}
