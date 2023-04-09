#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class SunShadowBlurOddPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(SunShadowBlurOddPass)

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
		TextureComponent *m_TextureComp;
		InnoMath::TVec4<uint32_t> m_numThreads;
		InnoMath::TVec4<uint32_t> m_numThreadGroups;
	};
} // namespace Inno