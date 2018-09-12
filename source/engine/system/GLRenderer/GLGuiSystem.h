#pragma once
#include "../../common/InnoType.h"
#include "../../third-party/ImGui/imgui.h"
#include "../../third-party/ImGui/imgui_impl_glfw_gl3.h"

namespace GLGuiSystem
{
	void setup();
	void initialize();
	void update();
	void shutdown();

	objectStatus m_GLGuiSystemStatus = objectStatus::SHUTDOWN;
};

