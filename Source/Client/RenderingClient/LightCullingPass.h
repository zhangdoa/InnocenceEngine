#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class LightCullingPassRenderingContext : public IRenderingContext
	{
		public:
		GPUResourceComponent *m_input;
	};

	class LightCullingPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(LightCullingPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassDataComponent *GetRPDC() override;

		GPUResourceComponent *GetResult();
		GPUResourceComponent* GetLightGrid();
		GPUResourceComponent* GetLightIndexList();
		GPUResourceComponent* GetHeatMap();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassDataComponent *m_RPDC;
		ShaderProgramComponent *m_SPC;
		SamplerDataComponent *m_SDC;

		GPUBufferDataComponent* m_lightListIndexCounter;
		GPUBufferDataComponent* m_lightIndexList;

		TextureDataComponent* m_lightGrid;
		TextureDataComponent* m_heatMap;

		const uint32_t m_tileSize = 16;
		InnoMath::TVec4<uint32_t> m_numThreads;
		InnoMath::TVec4<uint32_t> m_numThreadGroups;
	};
} // namespace Inno