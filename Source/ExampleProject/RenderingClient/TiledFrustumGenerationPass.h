#pragma once
#include "../../Engine/Interface/IRenderPass.h"
#include "LightCullingConstants.h"

namespace Inno
{
	class TiledFrustumGenerationPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(TiledFrustumGenerationPass)

		bool Setup(IServiceConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent *GetRenderPassComp() override;

		GPUResourceComponent *GetTiledFrustum();

	private:
		bool SetupFromRenderGraph();
		bool SetupImperative();
		bool SetupOwnedResources();

		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_ShaderProgramComp;
		GPUBufferComponent* m_TiledFrustum;
		GPUBufferComponent* m_DispatchParamsGPUBufferComp;
		const uint32_t m_tileSize = LightCulling::TILE_SIZE;
		const uint32_t m_numThreadPerGroup = LightCulling::TILE_SIZE;
		Math::TVec4<uint32_t> m_numThreads;
		Math::TVec4<uint32_t> m_numThreadGroups;

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
