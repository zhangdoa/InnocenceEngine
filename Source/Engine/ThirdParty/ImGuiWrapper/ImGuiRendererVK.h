#pragma once
#include "IImGuiRenderer.h"

namespace Inno
{
	class ImGuiRendererVK : public IImGuiRenderer
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(ImGuiRendererVK);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool NewFrame() override;
		bool Prepare() override;
		bool ExecuteCommands() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		void ShowRenderResult(RenderPassType renderPassType) override;
	};
}