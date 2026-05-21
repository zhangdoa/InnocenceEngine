#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class SSRCIntegrationPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(SSRCIntegrationPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		GPUResourceComponent* GetResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;
		TextureComponent* m_Result;

		ShaderStage m_ShaderStage;

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
