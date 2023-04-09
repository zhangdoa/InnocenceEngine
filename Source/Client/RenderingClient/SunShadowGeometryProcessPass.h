#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class SunShadowGeometryProcessPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(SunShadowGeometryProcessPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent *GetRenderPassComp() override;

		uint32_t GetShadowMapResolution();
		GPUResourceComponent *GetResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_SPC;
		SamplerComponent *m_SamplerComp;

		uint32_t m_shadowMapResolution = 1024;
	};
} // namespace Inno
