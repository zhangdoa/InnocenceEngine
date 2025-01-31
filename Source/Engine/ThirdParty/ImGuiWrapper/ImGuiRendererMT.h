#pragma once
#include "IImGuiRenderer.h"

namespace Inno
{
	class ImGuiRendererMT : public IImGuiRenderer
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(ImGuiRendererMT);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool NewFrame() override;
		bool Prepare() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		void ShowRenderResult(RenderPassType renderPassType) override;
	};
}
