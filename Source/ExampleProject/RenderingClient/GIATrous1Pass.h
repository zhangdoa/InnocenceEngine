#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// SVGF à-trous iteration 1 (stride 1). Consumes the temporal output
	// from GIDenoisePass + the moments texture; produces the first
	// spatially-filtered GI irradiance. Feeds GIATrous2Pass.
	class GIATrous1Pass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(GIATrous1Pass)

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
