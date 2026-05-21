#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// GI-1.0 §2.4.3 paper-faithful spatial filter — vertical axis of the separable
	// variable-radius bilateral blur. Writes per-pixel irradiance (rgb / max(N, 1), 1).
	class GIFilterVerticalPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(GIFilterVerticalPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		// Clears m_Result to zero so LightPass reads no GI when RasterizedGI toggles OFF
		// (avoids the stale-output artifact a plain bypass would leave).
		bool RecordClearCommandList(IRenderingContext* renderingContext = nullptr) override;
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
