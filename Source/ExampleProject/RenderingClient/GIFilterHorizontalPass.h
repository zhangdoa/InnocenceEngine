#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// GI-1.0 §2.4.3 paper-faithful spatial filter — horizontal axis of
	// the separable variable-radius bilateral blur (Capsaicin gi1.comp:
	// 4104–4154 FilterGI, gi1.cpp:2882–2927 dispatch wiring). Reads the
	// sample-count-weighted GI history from GIDenoisePass + the per-pixel
	// blur mask, writes a transient (rgb·N, N) scratch consumed by
	// GIFilterVerticalPass within the same frame.
	class GIFilterHorizontalPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(GIFilterHorizontalPass)

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
