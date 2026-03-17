#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class VXGIVisualizationPassRenderingContext: public IRenderingContext
	{
	public:
		GPUResourceComponent* m_input;
		uint32_t m_resolution;
	};

	class VXGIVisualizationPass: public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(VXGIVisualizationPass)

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent *GetRenderPassComp() override;

		GPUResourceComponent *GetResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_ShaderProgramComp;
	};
} // namespace Inno
