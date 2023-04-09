#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class VXGIConvertPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(VXGIConvertPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent *GetRenderPassComp() override;

		GPUResourceComponent *GetLuminanceVolume();
		GPUResourceComponent *GetNormalVolume();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_SPC;
		TextureComponent *m_TextureComp;
		TextureComponent *m_luminanceVolume;
		TextureComponent *m_normalVolume;
	};
} // namespace Inno
