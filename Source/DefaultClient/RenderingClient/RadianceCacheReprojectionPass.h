#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class RadianceCacheReprojectionPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(RadianceCacheReprojectionPass)

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		TextureComponent* GetCurrentFrameResult();
		TextureComponent* GetPreviousFrameResult();
		GPUBufferComponent* GetWorldProbeGrid();
		TextureComponent* GetCurrentProbePosition();
		TextureComponent* GetPreviousProbePosition();
		TextureComponent* GetCurrentProbeNormal();
		TextureComponent* GetPreviousProbeNormal();

	private:
		const uint32_t TILE_SIZE = 8;

		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;
		TextureComponent* m_RadianceCache_Odd;
		TextureComponent* m_RadianceCache_Even;
		TextureComponent* m_ProbePosition_Odd;
		TextureComponent* m_ProbePosition_Even;
		TextureComponent* m_ProbeNormal_Odd;
		TextureComponent* m_ProbeNormal_Even;

		GPUBufferComponent* m_WorldProbeGrid;

		ShaderStage m_ShaderStage;

		bool RenderTargetsCreationFunc();

	};
} // namespace Inno
