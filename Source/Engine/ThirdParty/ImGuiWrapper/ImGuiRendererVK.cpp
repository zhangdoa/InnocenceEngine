#include "ImGuiRendererVK.h"

#include "../../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiRendererVKNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

bool ImGuiRendererVK::setup()
{
	ImGuiRendererVKNS::m_ObjectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererVK setup finished.");

	return true;
}

bool ImGuiRendererVK::initialize()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererVK has been initialized.");

	return true;
}

bool ImGuiRendererVK::newFrame()
{
	return true;
}

bool ImGuiRendererVK::render()
{
	return true;
}

bool ImGuiRendererVK::terminate()
{
	ImGuiRendererVKNS::m_ObjectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiRendererVK has been terminated.");

	return true;
}

ObjectStatus ImGuiRendererVK::getStatus()
{
	return ImGuiRendererVKNS::m_ObjectStatus;
}

void ImGuiRendererVK::showRenderResult(RenderPassType renderPassType)
{
}