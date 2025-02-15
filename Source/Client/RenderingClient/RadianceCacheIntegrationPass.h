#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class RadianceCacheIntegrationPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(RadianceCacheIntegrationPass)

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		GPUResourceComponent* GetResult();

	private:
		const uint32_t TILE_SIZE = 8;
		const uint32_t SH_TILE_SIZE = 2;

		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;
		TextureComponent* m_Result;

		ShaderStage m_ShaderStage;

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
