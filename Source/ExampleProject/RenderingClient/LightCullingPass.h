#pragma once
#include "../../Engine/Interface/IRenderPass.h"
#include "LightCullingConstants.h"

namespace Inno
{
	class LightCullingPassRenderingContext : public IRenderingContext
	{
		public:
		GPUResourceComponent *m_input;
	};

	class LightCullingPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(LightCullingPass)

		bool Setup(IServiceConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;		
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent *GetRenderPassComp() override;

		GPUResourceComponent *GetResult();
		GPUResourceComponent* GetLightGrid();
		GPUResourceComponent* GetLightIndexList();
		GPUResourceComponent* GetHeatMap();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_ShaderProgramComp;
		SamplerComponent *m_SamplerComp;

		GPUBufferComponent* m_lightListIndexCounter;
		GPUBufferComponent* m_lightIndexList;
		GPUBufferComponent* m_DispatchParamsGPUBufferComp;

		TextureComponent* m_lightGrid;
		TextureComponent* m_heatMap;

		const uint32_t m_tileSize = LightCulling::TILE_SIZE;
		Math::TVec4<uint32_t> m_numThreads;
		Math::TVec4<uint32_t> m_numThreadGroups;

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno