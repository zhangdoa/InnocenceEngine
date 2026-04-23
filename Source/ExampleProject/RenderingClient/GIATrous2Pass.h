#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// SVGF à-trous iteration 2 (stride 2). Consumes GIATrous1Pass output
	// and the moments texture; produces the next spatial filter level.
	class GIATrous2Pass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(GIATrous2Pass)

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
