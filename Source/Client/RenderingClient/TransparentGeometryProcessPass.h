#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class TransparentGeometryProcessPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(TransparentGeometryProcessPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent *GetRenderPassComp() override;
		
		GPUBufferComponent *GetResultChannel0();
		GPUBufferComponent *GetResultChannel1();
		TextureComponent *GetHeadPtrTexture();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_ShaderProgramComp;

		GPUBufferComponent* m_atomicCounterGPUBufferComp;
		GPUBufferComponent* m_RT0;
		GPUBufferComponent* m_RT1;
		TextureComponent* m_HeadPtr;

		bool DepthStencilRenderTargetsReservationFunc();
		bool DepthStencilRenderTargetsCreationFunc();
	};
} // namespace Inno
