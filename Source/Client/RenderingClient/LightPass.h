#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class LightPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(LightPass)

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		TextureComponent* GetLuminanceResult();
		TextureComponent* GetIlluminanceResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;
		SamplerComponent* m_SamplerComp_Linear;
		SamplerComponent* m_SamplerComp_Point;

		TextureComponent* m_LuminanceResult;
		TextureComponent* m_IlluminanceResult;

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
