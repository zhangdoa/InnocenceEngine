#pragma once
#include "../../interface/IGuiSystem.h"
#include "../../interface/ILogSystem.h"

#include "../../third-party/ImGui/imgui.h"
#include "../../third-party/ImGui/imgui_impl_glfw_gl3.h"
#include "../../common/GLHeaders.h"

#include "../../component/WindowSystemSingletonComponent.h"
#include "../../component/ShadowRenderPassSingletonComponent.h"
#include "../../component/GeometryRenderPassSingletonComponent.h"
#include "../../component/LightRenderPassSingletonComponent.h"
#include "../../component/GLFinalRenderPassSingletonComponent.h"
#include "../../component/RenderingSystemSingletonComponent.h"

extern ILogSystem* g_pLogSystem;

class GLGuiSystem : public IGuiSystem
{
public:
	GLGuiSystem() {};
	~GLGuiSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};

