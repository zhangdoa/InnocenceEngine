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
		RenderPassComponent *GetRenderPassComp() override;

		GPUResourceComponent *GetResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_SPC;

		TextureComponent *m_oddTextureComp;
		TextureComponent *m_evenTextureComp;
		bool m_isPassOdd = true;
	};
} // namespace Inno
