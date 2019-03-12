#include "DXGuiSystem.h"
#include "../../component/DXWindowSystemComponent.h"
#include "../../component/DXRenderingSystemComponent.h"
#include "../../component/DXGeometryRenderPassComponent.h"

#include "../ImGuiWrapper.h"
#include "../../third-party/ImGui/imgui_impl_win32.h"
#include "../../third-party/ImGui/imgui_impl_dx11.h"
#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE DXGuiSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	void showRenderResult();
	ImTextureID getFileExplorerIconTextureID(const FileExplorerIconType iconType);

	std::function<void()> f_ShowRenderPassResult;
	std::function<ImTextureID(const FileExplorerIconType)> f_GetFileExplorerIconTextureID;
}

bool DXGuiSystem::setup()
{
	DXGuiSystemNS::f_ShowRenderPassResult = DXGuiSystemNS::showRenderResult;
	DXGuiSystemNS::f_GetFileExplorerIconTextureID = DXGuiSystemNS::getFileExplorerIconTextureID;

	ImGuiWrapper::get().setup();

	ImGuiWrapper::get().addShowRenderPassResultCallback(&DXGuiSystemNS::f_ShowRenderPassResult);
	ImGuiWrapper::get().addGetFileExplorerIconTextureIDCallback(&DXGuiSystemNS::f_GetFileExplorerIconTextureID);

	DXGuiSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

bool DXGuiSystem::initialize()
{
	ImGui_ImplWin32_Init(DXWindowSystemComponent::get().m_hwnd);
	ImGui_ImplDX11_Init(DXRenderingSystemComponent::get().m_device, DXRenderingSystemComponent::get().m_deviceContext);

	ImGuiWrapper::get().initialize();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXGuiSystem has been initialized.");
	return true;
}

bool DXGuiSystem::update()
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

bool DXGuiSystem::terminate()
{
	DXGuiSystemNS::m_objectStatus = ObjectStatus::STANDBY;

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();

	ImGuiWrapper::get().terminate();

	DXGuiSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXGuiSystem has been terminated.");
	return true;
}

ObjectStatus DXGuiSystem::getStatus()
{
	return DXGuiSystemNS::m_objectStatus;
}

void DXGuiSystemNS::showRenderResult()
{
	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();
	auto l_renderTargetSize = ImVec2((float)l_screenResolution.x / 4.0f, (float)l_screenResolution.y / 4.0f);

	ImGui::Begin("Opaque Pass", 0, ImGuiWindowFlags_AlwaysAutoResize);
	{
		{
			ImGui::BeginChild("World Space Position(RGB) + Metallic(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("World Space Position(RGB) + Metallic(A)");
			ImGui::Image(ImTextureID(DXGeometryRenderPassComponent::get().m_opaquePass_DXRPC->m_DXTDCs[0]->m_SRV), l_renderTargetSize, ImVec2(0.0, 0.0), ImVec2(1.0, 1.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("World Space Normal(RGB) + Roughness(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("World Space Normal(RGB) + Roughness(A)");
			ImGui::Image(ImTextureID(DXGeometryRenderPassComponent::get().m_opaquePass_DXRPC->m_DXTDCs[1]->m_SRV), l_renderTargetSize, ImVec2(0.0, 0.0), ImVec2(1.0, 1.0));
			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("Albedo(RGB) + Ambient Occlusion(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Albedo(RGB) + Ambient Occlusion(A)");
			ImGui::Image(ImTextureID(DXGeometryRenderPassComponent::get().m_opaquePass_DXRPC->m_DXTDCs[2]->m_SRV), l_renderTargetSize, ImVec2(0.0, 0.0), ImVec2(1.0, 1.0));
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Screen Space Motion Vector(RGB) + Transparency(A)", l_renderTargetSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::Text("Screen Space Motion Vector(RGB) + Transparency(A)");
			ImGui::Image(ImTextureID(DXGeometryRenderPassComponent::get().m_opaquePass_DXRPC->m_DXTDCs[3]->m_SRV), l_renderTargetSize, ImVec2(0.0, 0.0), ImVec2(1.0, 1.0));
			ImGui::EndChild();
		}
	}
	ImGui::End();
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