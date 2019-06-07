#include "ImGuiWrapperLinuxGL.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE ImGuiWrapperLinuxGLNS
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
}

bool ImGuiWrapperLinuxGL::setup()
{
	ImGuiWrapperLinuxGLNS::m_objectStatus = ObjectStatus::Created;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperLinuxGL setup finished.");

	return true;
}

bool ImGuiWrapperLinuxGL::initialize()
{
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperLinuxGL has been initialized.");

	return true;
}

bool ImGuiWrapperLinuxGL::newFrame()
{
	return true;
}

bool ImGuiWrapperLinuxGL::render()
{
	return true;
}

bool ImGuiWrapperLinuxGL::terminate()
{
	ImGuiWrapperLinuxGLNS::m_objectStatus = ObjectStatus::Terminated;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "ImGuiWrapperLinuxGL has been terminated.");

	return true;
}

ObjectStatus ImGuiWrapperLinuxGL::getStatus()
{
	return ImGuiWrapperLinuxGLNS::m_objectStatus;
}

void ImGuiWrapperLinuxGL::showRenderResult(RenderPassType renderPassType)
{
}

ImTextureID ImGuiWrapperLinuxGL::getFileExplorerIconTextureID(const FileExplorerIconType iconType)
{
	return nullptr;
}
