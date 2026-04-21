#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class LightPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(LightPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		TextureComponent* GetLuminanceResult();
		TextureComponent* GetIlluminanceResult();
		TextureComponent* GetCurrentGIHistory();
		TextureComponent* GetPreviousGIHistory();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;
		SamplerComponent* m_SamplerComp_Linear;
		SamplerComponent* m_SamplerComp_Point;

		TextureComponent* m_LuminanceResult;
		TextureComponent* m_IlluminanceResult;
		// Ping-pong full-screen GI irradiance history for the [I.3] temporal
		// denoiser. rgb = accumulated irradiance, a = linear depth at the
		// sampled pixel so the next frame can reject reprojections that
		// land on a surface at a different depth.
		TextureComponent* m_GIHistory_Even;
		TextureComponent* m_GIHistory_Odd;

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
