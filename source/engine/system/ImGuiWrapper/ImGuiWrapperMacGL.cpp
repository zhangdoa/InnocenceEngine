#include "ImGuiWrapperMacGL.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE ImGuiWrapperMacGLNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
}

bool ImGuiWrapperMacGL::setup()
{
	ImGuiWrapperMacGLNS::m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperMacGL setup finished.");

	return true;
}

bool ImGuiWrapperMacGL::initialize()
{
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperMacGL has been initialized.");

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
	ImGuiWrapperMacGLNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperMacGL has been terminated.");

	return true;
}

ObjectStatus ImGuiWrapperMacGL::getStatus()
{
	return ImGuiWrapperMacGLNS::m_objectStatus;
}

void ImGuiWrapperMacGL::showRenderResult()
{
}

ImTextureID ImGuiWrapperMacGL::getFileExplorerIconTextureID(const FileExplorerIconType iconType)
{
	return nullptr;
}
