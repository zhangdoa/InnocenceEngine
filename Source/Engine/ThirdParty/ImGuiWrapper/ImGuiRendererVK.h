#pragma once
#include "IImGuiRenderer.h"

class ImGuiRendererVK : public IImGuiRenderer
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(ImGuiRendererVK);

	bool Setup(ISystemConfig* systemConfig) override;
	bool Initialize() override;
	bool NewFrame() override;
	bool Render() override;
	bool Terminate() override;

	ObjectStatus GetStatus() override;

	void ShowRenderResult(RenderPassType renderPassType) override;
};