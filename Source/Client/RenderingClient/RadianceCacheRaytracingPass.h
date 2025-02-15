#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class RadianceCacheRaytracingPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(RadianceCacheRaytracingPass)

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

	private:
		const uint32_t TILE_SIZE = 8;

		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;
		SamplerComponent* m_SamplerComp;

		ShaderStage m_ShaderStage;

		bool RenderTargetsCreationFunc();		
	};
} // namespace Inno
