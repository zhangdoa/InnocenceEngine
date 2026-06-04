#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// GI-1.0 §2.4.3 paper-faithful spatial filter — horizontal axis of
	// the separable variable-radius bilateral blur (Capsaicin gi1.comp:
	// 4104–4154 FilterGI, gi1.cpp:2882–2927 dispatch wiring). Reads the
	// sample-count-weighted GI history from SSRCTemporalPass + the per-pixel
	// blur mask, writes a transient (rgb·N, N) scratch consumed by
	// SSRCSpatialVerticalPass within the same frame.
	class SSRCSpatialHorizontalPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(SSRCSpatialHorizontalPass)

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
