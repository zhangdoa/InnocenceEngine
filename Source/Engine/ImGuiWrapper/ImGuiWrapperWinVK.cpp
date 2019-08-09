#include "ImGuiWrapperWinVK.h"
#include "../Component/WinWindowSystemComponent.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiWrapperWinVKNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
}

bool ImGuiWrapperWinVK::setup()
{
	ImGuiWrapperWinVKNS::m_objectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWinVK setup finished.");

	return true;
}

bool ImGuiWrapperWinVK::initialize()
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWinVK has been initialized.");

	return true;
}

bool ImGuiWrapperWinVK::newFrame()
{
	return true;
}

bool ImGuiWrapperWinVK::render()
{
	return true;
}

bool ImGuiWrapperWinVK::terminate()
{
	ImGuiWrapperWinVKNS::m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWrapperWinVK has been terminated.");

	return true;
}

ObjectStatus ImGuiWrapperWinVK::getStatus()
{
	return ImGuiWrapperWinVKNS::m_objectStatus;
}

void ImGuiWrapperWinVK::showRenderResult(RenderPassType renderPassType)
{
}

ImTextureID ImGuiWrapperWinVK::getFileExplorerIconTextureID(const FileExplorerIconType iconType)
{
	return nullptr;
}