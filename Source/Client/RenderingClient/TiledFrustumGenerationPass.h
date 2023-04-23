#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class TiledFrustumGenerationPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(TiledFrustumGenerationPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent *GetRenderPassComp() override;

		GPUResourceComponent *GetTiledFrustum();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_ShaderProgramComp;
		GPUBufferComponent* m_tiledFrustum;
		const uint32_t m_tileSize = 16;
		const uint32_t m_numThreadPerGroup = 16;
		Math::TVec4<uint32_t> m_numThreads;
		Math::TVec4<uint32_t> m_numThreadGroups;

		bool CreateResources();
		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
