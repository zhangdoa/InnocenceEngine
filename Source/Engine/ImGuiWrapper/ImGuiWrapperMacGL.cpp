#include "ImGuiWrapperMacGL.h"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

INNO_PRIVATE_SCOPE ImGuiWrapperMacGLNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
}

bool ImGuiWrapperMacGL::setup()
{
	ImGuiWrapperMacGLNS::m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperMacGL setup finished.");

	return true;
}

bool ImGuiWrapperMacGL::initialize()
{
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperMacGL has been initialized.");

	return true;
}

bool ImGuiWrapperMacGL::newFrame()
{
	return true;
}

bool ImGuiWrapperMacGL::render()
{
	return true;
}

bool ImGuiWrapperMacGL::terminate()
{
	ImGuiWrapperMacGLNS::m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperMacGL has been terminated.");

	return true;
}

ObjectStatus ImGuiWrapperMacGL::getStatus()
{
	return ImGuiWrapperMacGLNS::m_objectStatus;
}

void ImGuiWrapperMacGL::showRenderResult(RenderPassType renderPassType)
{
}

ImTextureID ImGuiWrapperMacGL::getFileExplorerIconTextureID(const FileExplorerIconType iconType)
{
	return nullptr;
}
