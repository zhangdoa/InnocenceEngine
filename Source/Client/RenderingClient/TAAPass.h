#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{	
	class TAAPassRenderingContext : public IRenderingContext
	{
		public:
		GPUResourceComponent *m_input;
	};

	class TAAPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(TAAPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassDataComponent *GetRPDC() override;

		GPUResourceComponent *GetResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassDataComponent *m_RPDC;
		ShaderProgramComponent *m_SPC;

		TextureDataComponent *m_oddTDC;
		TextureDataComponent *m_evenTDC;
		bool m_isPassOdd = true;
	};
} // namespace Inno
