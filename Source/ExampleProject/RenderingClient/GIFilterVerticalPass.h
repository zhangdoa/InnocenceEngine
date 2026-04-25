#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// GI-1.0 §2.4.3 paper-faithful spatial filter — vertical axis of the
	// separable variable-radius bilateral blur. Reads the horizontal
	// pass's scratch + the per-pixel blur mask; writes the final
	// per-pixel irradiance (rgb / max(N, 1), 1) consumed by LightPass.
	// Replaces GIATrous4Pass as LightPass's GI source.
	class GIFilterVerticalPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(GIFilterVerticalPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		TextureComponent* GetResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;
		TextureComponent* m_Result;

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
