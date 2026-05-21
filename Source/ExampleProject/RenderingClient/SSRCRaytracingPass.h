#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class SSRCRaytracingPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(SSRCRaytracingPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

	private:
		// Spawn-tile constants live in SSRCConstants.h so the C++
		// dispatch grid and the HLSL upscaleFactor (RayTracingTypes.hlsl)
		// stay in lockstep. Ray-gen runs one thread per spawn tile, one probe
		// spawned per frame at a Halton-picked sub-pixel.
		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;
		SamplerComponent* m_SamplerComp;

		ShaderStage m_ShaderStage;

		bool RenderTargetsCreationFunc();		
	};
} // namespace Inno
