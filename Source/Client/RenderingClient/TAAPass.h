#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class TAAPassRenderingContext : public IRenderingContext
	{
	public:
		GPUResourceComponent* m_input;
		GPUResourceComponent* m_motionVector;
		GPUResourceComponent* m_readTexture;
		GPUResourceComponent* m_writeTexture;
	};

	class TAAPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(TAAPass)

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		GPUResourceComponent* GetOddResult();
		GPUResourceComponent* GetEvenResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;

		TextureComponent* m_OddTextureComp;
		TextureComponent* m_EvenTextureComp;

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
