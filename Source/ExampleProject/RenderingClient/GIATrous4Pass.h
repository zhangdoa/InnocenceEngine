#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// SVGF à-trous iteration 3 (stride 4). Final spatial filter stage —
	// LightPass reads this pass's output as the denoised GI irradiance.
	class GIATrous4Pass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(GIATrous4Pass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		TextureComponent* GetResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;
		TextureComponent* m_Result;

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
