#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class LuminanceAveragePass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(LuminanceAveragePass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update();
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent *GetRenderPassComp() override;

		GPUResourceComponent *GetResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_ShaderProgramComp;
		uint32_t m_MaxResultToKeep = 8;

		GPUBufferComponent *m_luminanceAverage = 0;
	};
} // namespace Inno
