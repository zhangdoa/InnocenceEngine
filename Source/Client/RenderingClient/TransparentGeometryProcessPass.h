#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class TransparentGeometryProcessPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(TransparentGeometryProcessPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassDataComponent *GetRPDC() override;
		
		GPUBufferDataComponent *GetResultChannel0();
		GPUBufferDataComponent *GetResultChannel1();
		TextureDataComponent *GetHeadPtrTexture();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassDataComponent *m_RPDC;
		ShaderProgramComponent *m_SPC;

		GPUBufferDataComponent* m_atomicCounterGBDC;
		GPUBufferDataComponent* m_RT0;
		GPUBufferDataComponent* m_RT1;
		TextureDataComponent* m_HeadPtr;
	};
} // namespace Inno
